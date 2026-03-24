from __future__ import annotations

import asyncio
import json
import os
import time
from collections import deque
from contextlib import suppress
from pathlib import Path
from typing import Any

from fastapi import FastAPI, HTTPException, WebSocket, WebSocketDisconnect
from fastapi.middleware.cors import CORSMiddleware
from pydantic import BaseModel, ConfigDict, Field


def resolve_project_root(project_root: Path | None = None) -> Path:
    if project_root is not None:
        return project_root.resolve()
    configured_root = os.environ.get("DEVICELAB_PROJECT_ROOT")
    if configured_root:
        return Path(configured_root).resolve()
    return Path(__file__).resolve().parents[2]


def load_json(path: Path) -> dict[str, Any]:
    return json.loads(path.read_text(encoding="utf-8"))


class ScenarioDescriptor(BaseModel):
    model_config = ConfigDict(extra="ignore")

    id: str
    name: str
    description: str = ""
    profile_id: str = ""
    duration_ticks: int = Field(ge=1)
    ui_rank: int = Field(default=99, ge=0)
    file_name: str


class ProfileDescriptor(BaseModel):
    model_config = ConfigDict(extra="ignore")

    id: str
    name: str
    description: str = ""
    target_temperature_c: float | None = None
    critical_temperature_c: float | None = None
    file_name: str


class AssertionSnapshot(BaseModel):
    model_config = ConfigDict(extra="ignore")

    tick: int
    passed: bool
    description: str


class ScenarioSummaryInfo(BaseModel):
    model_config = ConfigDict(extra="ignore")

    id: str
    name: str
    description: str = ""
    profile_id: str = ""


class ProfileSummaryInfo(BaseModel):
    model_config = ConfigDict(extra="ignore")

    id: str
    name: str
    description: str = ""


class RunSummarySnapshot(BaseModel):
    model_config = ConfigDict(extra="ignore")

    run_id: str
    passed: bool
    digest: str
    final_state: str
    final_faults: list[str] = Field(default_factory=list)
    assertion_pass_count: int = 0
    assertion_fail_count: int = 0
    scenario: ScenarioSummaryInfo
    profile: ProfileSummaryInfo
    assertions: list[AssertionSnapshot] = Field(default_factory=list)


class RunHistoryItem(BaseModel):
    model_config = ConfigDict(extra="ignore")

    run_id: str
    scenario_id: str
    scenario_name: str
    passed: bool
    digest: str
    final_state: str
    artifact_dir: str


class ActiveRunStatus(BaseModel):
    model_config = ConfigDict(extra="ignore")

    scenario_id: str
    profile_id: str
    delay_ms: int = Field(ge=25, le=2000)
    artifact_dir: str
    started_epoch_s: int


class RunRequest(BaseModel):
    profile_id: str | None = None
    delay_ms: int = Field(default=160, ge=25, le=2000)


class BridgeHealth(BaseModel):
    status: str
    binary_available: bool
    scenario_count: int
    profile_count: int
    history_count: int


class OverviewResponse(BaseModel):
    binary_available: bool
    active_run: ActiveRunStatus | None = None
    current_snapshot: dict[str, Any] | None = None
    telemetry_history: list[dict[str, Any]] = Field(default_factory=list)
    recent_logs: list[dict[str, Any]] = Field(default_factory=list)
    recent_transitions: list[dict[str, Any]] = Field(default_factory=list)
    latest_summary: RunSummarySnapshot | None = None
    recent_summaries: list[RunSummarySnapshot] = Field(default_factory=list)
    scenarios: list[ScenarioDescriptor] = Field(default_factory=list)
    profiles: list[ProfileDescriptor] = Field(default_factory=list)
    history: list[RunHistoryItem] = Field(default_factory=list)


class BridgeState:
    def __init__(self, project_root: Path) -> None:
        self.project_root = project_root
        self.scenarios_dir = self.project_root / "scenarios"
        self.profiles_dir = self.project_root / "profiles"
        self.runs_dir = self.project_root / "runs"
        self.runs_dir.mkdir(parents=True, exist_ok=True)

        self.connections: set[WebSocket] = set()
        self.telemetry_history: deque[dict[str, Any]] = deque(maxlen=180)
        self.recent_logs: deque[dict[str, Any]] = deque(maxlen=160)
        self.recent_transitions: deque[dict[str, Any]] = deque(maxlen=64)
        self.recent_summaries: deque[RunSummarySnapshot] = deque(maxlen=20)
        self.current_snapshot: dict[str, Any] | None = None
        self.latest_summary: RunSummarySnapshot | None = None
        self.active_run: ActiveRunStatus | None = None
        self.process: asyncio.subprocess.Process | None = None
        self.stdout_task: asyncio.Task[None] | None = None
        self.stderr_task: asyncio.Task[None] | None = None
        self.lock = asyncio.Lock()

        for summary in self.load_summary_history(limit=5, include_full_summary=True):
            self.recent_summaries.append(summary)
        self.latest_summary = self.recent_summaries[0] if self.recent_summaries else None

    def binary_candidates(self) -> list[Path]:
        env = os.environ.get("DEVICELAB_CLI_BIN")
        candidates = [
            Path(env).resolve() if env else None,
            self.project_root / "build" / "devicelab_cli",
            self.project_root / "build" / "Debug" / "devicelab_cli",
            self.project_root / "build" / "Release" / "devicelab_cli",
            self.project_root / "bin" / "devicelab_cli",
        ]
        return [candidate for candidate in candidates if candidate is not None]

    def cli_binary(self) -> Path | None:
        for candidate in self.binary_candidates():
            if candidate.exists() and candidate.is_file():
                return candidate
        return None

    def scenario_catalog(self) -> list[ScenarioDescriptor]:
        scenarios: list[ScenarioDescriptor] = []
        for path in sorted(self.scenarios_dir.glob("*.json")):
            data = load_json(path)
            scenarios.append(
                ScenarioDescriptor.model_validate(
                    {
                        "id": data["id"],
                        "name": data["name"],
                        "description": data.get("description", ""),
                        "profile_id": data.get("profile_id", ""),
                        "duration_ticks": data["duration_ticks"],
                        "ui_rank": data.get("ui_rank", 99),
                        "file_name": path.name,
                    }
                )
            )
        return sorted(scenarios, key=lambda scenario: (scenario.ui_rank, scenario.name))

    def profile_catalog(self) -> list[ProfileDescriptor]:
        profiles: list[ProfileDescriptor] = []
        for path in sorted(self.profiles_dir.glob("*.json")):
            data = load_json(path)
            profiles.append(
                ProfileDescriptor.model_validate(
                    {
                        "id": data["id"],
                        "name": data["name"],
                        "description": data.get("description", ""),
                        "target_temperature_c": data.get("target_temperature_c"),
                        "critical_temperature_c": data.get("critical_temperature_c"),
                        "file_name": path.name,
                    }
                )
            )
        return profiles

    def load_summary_history(
        self,
        *,
        limit: int = 20,
        include_full_summary: bool = False,
    ) -> list[RunHistoryItem] | list[RunSummarySnapshot]:
        items: list[RunHistoryItem] | list[RunSummarySnapshot] = []
        paths = sorted(
            self.runs_dir.glob("*/summary.json"),
            key=lambda item: item.stat().st_mtime,
            reverse=True,
        )
        for path in paths[:limit]:
            data = load_json(path)
            if include_full_summary:
                items.append(RunSummarySnapshot.model_validate(data))
            else:
                items.append(
                    RunHistoryItem.model_validate(
                        {
                            "run_id": data["run_id"],
                            "scenario_id": data["scenario"]["id"],
                            "scenario_name": data["scenario"]["name"],
                            "passed": data["passed"],
                            "digest": data["digest"],
                            "final_state": data["final_state"],
                            "artifact_dir": path.parent.name,
                        }
                    )
                )
        return items

    def run_history(self) -> list[RunHistoryItem]:
        return list(self.load_summary_history())

    def locate_scenario(self, scenario_id: str) -> Path:
        for path in self.scenarios_dir.glob("*.json"):
            data = load_json(path)
            if data["id"] == scenario_id:
                return path
        raise FileNotFoundError(f"Unknown scenario '{scenario_id}'")

    def locate_profile(self, profile_id: str) -> Path:
        for path in self.profiles_dir.glob("*.json"):
            data = load_json(path)
            if data["id"] == profile_id:
                return path
        raise FileNotFoundError(f"Unknown profile '{profile_id}'")

    async def broadcast(self, payload: dict[str, Any]) -> None:
        stale: list[WebSocket] = []
        for connection in self.connections:
            try:
                await connection.send_json(payload)
            except Exception:
                stale.append(connection)
        for connection in stale:
            self.connections.discard(connection)

    async def stop_existing_run(self) -> None:
        if self.process is None:
            return
        if self.process.returncode is None:
            self.process.terminate()
            try:
                await asyncio.wait_for(self.process.wait(), timeout=5)
            except asyncio.TimeoutError:
                self.process.kill()
                await self.process.wait()
        for task in (self.stdout_task, self.stderr_task):
            if task is not None:
                task.cancel()
                with suppress(asyncio.CancelledError):
                    await task
        self.process = None
        self.stdout_task = None
        self.stderr_task = None
        self.active_run = None

    async def start_run(self, scenario_id: str, request: RunRequest) -> ActiveRunStatus:
        async with self.lock:
            await self.stop_existing_run()

            binary = self.cli_binary()
            if binary is None:
                raise FileNotFoundError(
                    "DeviceLab CLI binary not found. Build with CMake or set DEVICELAB_CLI_BIN."
                )

            scenario_path = self.locate_scenario(scenario_id)
            scenario_data = load_json(scenario_path)
            selected_profile_id = request.profile_id or scenario_data.get("profile_id")
            if not selected_profile_id:
                raise FileNotFoundError("Scenario does not define a profile_id and none was provided")
            profile_path = self.locate_profile(selected_profile_id)

            artifact_dir = f"live-{scenario_id}-{time.strftime('%Y%m%d-%H%M%S')}"
            run_dir = self.runs_dir / artifact_dir
            self.telemetry_history.clear()
            self.recent_logs.clear()
            self.recent_transitions.clear()
            self.current_snapshot = None
            self.latest_summary = None
            self.active_run = ActiveRunStatus(
                scenario_id=scenario_id,
                profile_id=selected_profile_id,
                delay_ms=request.delay_ms,
                artifact_dir=artifact_dir,
                started_epoch_s=int(time.time()),
            )

            self.process = await asyncio.create_subprocess_exec(
                str(binary),
                "stream",
                "--scenario",
                str(scenario_path),
                "--profile",
                str(profile_path),
                "--output-dir",
                str(run_dir),
                "--delay-ms",
                str(request.delay_ms),
                cwd=str(self.project_root),
                stdout=asyncio.subprocess.PIPE,
                stderr=asyncio.subprocess.PIPE,
            )
            self.stdout_task = asyncio.create_task(self._consume_stdout())
            self.stderr_task = asyncio.create_task(self._consume_stderr())

        await self.broadcast({"type": "run_started", "payload": self.active_run.model_dump(mode="json")})
        return self.active_run

    async def _consume_stdout(self) -> None:
        assert self.process is not None
        assert self.process.stdout is not None
        while True:
            line = await self.process.stdout.readline()
            if not line:
                break
            try:
                payload = json.loads(line.decode("utf-8"))
            except json.JSONDecodeError as error:
                await self.broadcast({"type": "stderr", "payload": {"message": f"Invalid JSON stream: {error}"}})
                continue

            message_type = payload.get("type")
            content = payload.get("payload")

            if message_type == "telemetry":
                self.current_snapshot = content
                self.telemetry_history.append(content)
            elif message_type == "log":
                self.recent_logs.append(content)
            elif message_type == "transition":
                self.recent_transitions.append(content)
            elif message_type == "summary":
                self.latest_summary = RunSummarySnapshot.model_validate(content)
                self.recent_summaries.appendleft(self.latest_summary)
                self.active_run = None
                payload["payload"] = self.latest_summary.model_dump(mode="json")

            await self.broadcast(payload)

        if self.process is not None:
            await self.process.wait()
            return_code = self.process.returncode
        else:
            return_code = 0

        if self.active_run is not None:
            await self.broadcast(
                {
                    "type": "run_finished",
                    "payload": {"status": "completed", "return_code": return_code},
                }
            )
            self.active_run = None

    async def _consume_stderr(self) -> None:
        assert self.process is not None
        assert self.process.stderr is not None
        while True:
            line = await self.process.stderr.readline()
            if not line:
                break
            await self.broadcast({"type": "stderr", "payload": {"message": line.decode("utf-8").rstrip()}})

    def overview(self) -> OverviewResponse:
        return OverviewResponse(
            binary_available=self.cli_binary() is not None,
            active_run=self.active_run,
            current_snapshot=self.current_snapshot,
            telemetry_history=list(self.telemetry_history),
            recent_logs=list(self.recent_logs),
            recent_transitions=list(self.recent_transitions),
            latest_summary=self.latest_summary,
            recent_summaries=list(self.recent_summaries),
            scenarios=self.scenario_catalog(),
            profiles=self.profile_catalog(),
            history=self.run_history(),
        )

    def health(self) -> BridgeHealth:
        return BridgeHealth(
            status="ok",
            binary_available=self.cli_binary() is not None,
            scenario_count=len(list(self.scenarios_dir.glob("*.json"))),
            profile_count=len(list(self.profiles_dir.glob("*.json"))),
            history_count=len(list(self.runs_dir.glob("*/summary.json"))),
        )


def create_app(project_root: Path | None = None) -> FastAPI:
    bridge_state = BridgeState(resolve_project_root(project_root))
    app = FastAPI(title="DeviceLab Pro Bridge", version="1.1.0")
    app.state.bridge = bridge_state
    app.add_middleware(
        CORSMiddleware,
        allow_origins=["*"],
        allow_methods=["*"],
        allow_headers=["*"],
    )

    @app.get("/api/health", response_model=BridgeHealth)
    async def health() -> BridgeHealth:
        return bridge_state.health()

    @app.get("/api/overview", response_model=OverviewResponse)
    async def overview() -> OverviewResponse:
        return bridge_state.overview()

    @app.get("/api/scenarios", response_model=list[ScenarioDescriptor])
    async def scenarios() -> list[ScenarioDescriptor]:
        return bridge_state.scenario_catalog()

    @app.get("/api/profiles", response_model=list[ProfileDescriptor])
    async def profiles() -> list[ProfileDescriptor]:
        return bridge_state.profile_catalog()

    @app.get("/api/history", response_model=list[RunHistoryItem])
    async def history() -> list[RunHistoryItem]:
        return bridge_state.run_history()

    @app.post("/api/runs/{scenario_id}", response_model=ActiveRunStatus)
    async def start_run(scenario_id: str, request: RunRequest) -> ActiveRunStatus:
        try:
            return await bridge_state.start_run(scenario_id=scenario_id, request=request)
        except FileNotFoundError as error:
            raise HTTPException(status_code=404, detail=str(error)) from error

    @app.websocket("/ws/telemetry")
    async def telemetry_socket(websocket: WebSocket) -> None:
        await websocket.accept()
        bridge_state.connections.add(websocket)
        await websocket.send_json({"type": "bootstrap", "payload": bridge_state.overview().model_dump(mode="json")})
        try:
            while True:
                try:
                    await asyncio.wait_for(websocket.receive_text(), timeout=25)
                except asyncio.TimeoutError:
                    await websocket.send_json({"type": "heartbeat", "payload": {"epoch_s": int(time.time())}})
        except WebSocketDisconnect:
            bridge_state.connections.discard(websocket)

    return app


app = create_app()
