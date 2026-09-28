gviz is a graph layout engine and renderer. 

# Basics

When debugging it, you should look for critical bugs in code first. If something is incorrect, or does something different than what the code assumes, check it. You are able to take screenshots of runtime with renderer.RequestScreenshot("path/to/file.bmp"). Make sure to toggle on lock camera fit via Renderer::LockToFit().

Some issues may be due to misconfiguration. Consider that where it makes sense only. Though it is not common. For example, default values may not be fit for the graph being tested or are not initialized correctly.

# Sampling and Charting

Some issues, however, may not be obvious from the code but arise as runtime issues. With the GVIZ_DEBUG_CHARTS cmake flag ON, you will be able to easily chart and check any ComponentPool over time. 

# Steps

## Capture Samples
To capture samples from a specific component pool, it must be recorded. Get (or create) a ChartRecorderResource on the graph via graph.SetResource<ChartRecorderResource>(graph.NodeSpace()). The pointer .recorder.get() returns is the tool you will use to record samples of any component pool. 

Call recorder->PushFrame<YourComponent>() once per tick for every component type you want tracked. The running list of samples will be accessible through the recorder. Additionally, you can get the data rendered, instead of accessing it in code.

## Rendering Charts

On the renderer side, call renderer.DebugCharts().With<YourComponent>("label", decodeFn), where decodeFn takes const std::vector<YourComponent>& (all entities for one recorded frame) and returns a std::vector<ChartDataPoint> — typically one aggregated value (e.g. an average) per named series; each distinct name becomes its own line chart, and all charts render automatically each frame in an ImGui "Debug Charts" window as long as the renderer can see the same ChartRecorderResource on the graph. Call Remove<YourComponent>() to stop charting a type. None of this exists unless the project is built with GVIZ_DEBUG_CHARTS=ON.

Methods:
- ChartRecorder::PushFrame<T>() — records the current state of DenseComponentPool<T> as one new frame (no return value).
- ChartRecorder::Samples<T>() const — returns const std::vector<std::vector<T>>&, all recorded frames for T (outer = frame index, inner = per-entity data).
- ChartRecorder::SampleCount<T>() const — returns size_t, number of frames recorded for T.
- ChartWindow::With<T>(std::string label, ChartTrack<T>::DecodeFn decode) — registers/updates T for charting under label; no return value. decode signature: std::vector<ChartDataPoint>(const std::vector<T>&).
- ChartWindow::Remove<T>() — unregisters T from charting; no return value.
- ChartWindow::Clear() — unregisters everything; no return value.
- ChartWindow::Render(const ChartRecorder&, bool &open) — draws the ImGui window and all registered charts; no return value (mutates open if the window is closed via its X button).
- Renderer::DebugCharts() — returns ChartWindow& for the renderer's chart registry.
