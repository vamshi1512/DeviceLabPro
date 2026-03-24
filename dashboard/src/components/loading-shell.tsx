export function LoadingShell() {
  return (
    <div className="relative min-h-screen overflow-hidden px-4 py-6 md:px-8 lg:px-10">
      <div className="mx-auto max-w-[1600px]">
        <div className="panel min-h-[220px] animate-pulse p-8">
          <div className="h-4 w-40 rounded-full bg-white/10" />
          <div className="mt-6 h-12 max-w-2xl rounded-2xl bg-white/10" />
          <div className="mt-4 h-5 max-w-3xl rounded-full bg-white/5" />
          <div className="mt-10 grid gap-4 md:grid-cols-3">
            <div className="h-28 rounded-[24px] bg-white/5" />
            <div className="h-28 rounded-[24px] bg-white/5" />
            <div className="h-28 rounded-[24px] bg-white/5" />
          </div>
        </div>
        <div className="mt-6 grid gap-5 xl:grid-cols-12">
          <div className="panel h-[360px] animate-pulse xl:col-span-8" />
          <div className="panel h-[360px] animate-pulse xl:col-span-4" />
        </div>
      </div>
    </div>
  );
}
