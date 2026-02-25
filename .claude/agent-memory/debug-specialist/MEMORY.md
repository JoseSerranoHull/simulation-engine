# Debug Specialist Agent Memory

## Key Recurring Bug Patterns

### Checkerboard Pipeline: INI Key Mismatch (CONFIRMED 2026-02-25)
- SceneLoader::handleMaterial() reads `"GraphicsPipeline"` as the ini key (line 128 of SceneLoader.cpp)
- simulation_lab2.ini uses `"Pipeline"` as the key
- Result: pipeIdx silently defaults to 0 (Phong), Mat_Checker material stores Phong pipeline pointer
- Downstream: Renderer pointer comparison (Renderer.cpp line 194) can never succeed
- Downstream: Phong shader executes, samples white.png albedo, outputs solid white
- Downstream: ImGui changes to m_checkerConstants are never transmitted to GPU
- Fix: make the key consistent — either change ini to "GraphicsPipeline = 6" or change SceneLoader to read "Pipeline"

### Renderer Checkerboard Detection: Fragile Pointer Comparison
- Renderer::recordOpaquePass() detects checkerboard meshes by comparing sub.m_material->getPipeline() == checkerboardPipeline
- This is a leaky abstraction: silently fails if material pipeline pointer doesn't match (e.g. due to any ini/loader mismatch)
- Long-term fix: material should self-describe its extra push data (callback or virtual method)

### Pipeline Draw Path for Checkerboard
- Renderer passes pipelineOverride = nullptr to Mesh::draw() (Renderer.cpp line 203)
- Mesh::draw() resolves activePipeline from material->getPipeline() when override is nullptr
- This means the material's stored pipeline pointer is the actual pipeline that runs — not the checkerboard pipeline
- For checkerboard to work, material must store m_pipelines[6] pointer (not m_pipelines[0])

## Confirmed Correct Subsystems (Checkerboard)
- Scenario::createMaterialPipelines(): pipeline[6] created correctly with CHECKER_PC_SIZE=100, CHECKER_PC_STAGES=VERT|FRAG (Scenario.cpp line 57)
- GenericScenario::GetCheckerboardPipeline(): correctly returns m_pipelines[6].get() (GenericScenario.h line 35)
- EngineOrchestrator::drawFrame(): correctly extracts checkerPipeline/PushData/PushSize and passes to recordFrame (lines 256-270)
- Mesh::draw(): ExtraPushConstants path is correctly implemented (Mesh.cpp lines 80-83)
- checkerboard.vert/.frag: push constant layout is correct — model@0, colorA@64, colorB@80, scale@96
- CheckerboardPushConstants struct in GenericScenario.h: layout matches GLSL (alignas(16) on vec4s)

## File Path Quick Reference
- INI key mismatch: source/scene/SceneLoader.cpp line 128
- Renderer pipeline detection: source/graphics/Renderer.cpp line 194
- Pipeline creation: source/scene/Scenario.cpp line 57
- Checkerboard pipeline getter: include/scene/GenericScenario.h line 35
- Mesh draw with extra push: source/assets/Mesh.cpp lines 80-83
- Shaders: shaders/checkerboard.vert, shaders/checkerboard.frag
