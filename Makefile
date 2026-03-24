.PHONY: build test smoke bridge bridge-test dashboard dashboard-build report validate clean

build:
	cmake -S . -B build
	cmake --build build

test: build
	ctest --test-dir build --output-on-failure

smoke: build
	./build/devicelab_cli run --scenario scenarios/nominal_behavior.json --profile profiles/thermal_controller_mk1.json

bridge:
	python3 -m uvicorn tools.bridge.server:app --host 0.0.0.0 --port 8000

bridge-test:
	python3 -m pytest tools -q

dashboard:
	cd dashboard && npm run dev

dashboard-build:
	cd dashboard && npm run build

report:
	python3 tools/reporting/render_report.py runs/nominal_behavior/summary.json

validate: test smoke bridge-test dashboard-build

clean:
	rm -rf build dashboard/dist
