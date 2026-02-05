# Complete Real Time Graphics Lab Book (1 - 8)
---
## MSc Computer Science for Games Programming 
---

**Course: 700106 - Real-Time Graphics**
**Name: JOSE JAVIER SERRANO SOLIS**

September 2025 – January 2026 

Dr. Quingde Li, Dr. Xihui Ma

---

## Lab Book Chapter 1: Basic geometric modelling

This session is designed to build a foundational understanding of how to create, manage, and render simple 3D objects using a modern Vulkan 1.3 workflow.
The progression moves from rendering basic triangles to constructing and manipulating multiple objects in 3D space. By focusing on core components—such as vertex buffers,
pipelines with dynamic rendering, and uniform buffers—critical insights are provided into the structure of a modern graphics application and the explicit nature of the Vulkan API.

### Exercise 0: Code Familiarization

#### Question
Before writing any code, take a guided tour of the sample project to understand its structure:

- InitVulkan(): Instance, Device, Queues, Swapchain & Image Views.

- Asset Creation: Vertex/Index Data, Buffer Creation, Staging Buffer.

- Pipeline Setup: Pipeline Layout, Graphics Pipeline.

- RecordCommandBuffer(): Dynamic Rendering Commands, Resource Binding, Draw Call.

- drawFrame(): Synchronization.

#### Solution
![Solution 0](<../markdown-resources/Complete Real Time Graphics Lab Book (1 - 8)/img1-0.png> "Program running")

As shown on the Img. 0.1 I was able to get the program running in the laboratory’s PC. 
Also got to read the code as the instructions explained.

- Test data
N/A

- Sample output
A quad with color rainbow (due to the vertex color) doing a yaw rotation.

- Reflection
*Reflect on what you have learnt from this exercise.*
I learnt about the main structure of a C++ code that’s using the Vulkan API. Along 
with what does the system needs in order to function (like the installation of the API or 
the folder of the GLFW library next to the project’s folder). Also learned about the 
multiple methods that Copilot can help you.

*Did you make any mistakes?*
No as far as I know. As long as I properly created the project on Visual Studio all run smoothly.

*In what way has your knowledge improved?*
In the main parts of the code that I can edit in order to do the next couple of 
exercises and about the main structure of the code using the Vulkan API with all its 
methods in the proper execution order. And that Copilot is a powerful tool, that can 
appear and help me in diverse ways within Visual Studio.

- Questions

*Is there anything you would like to ask?*
No.

### Exercise 1: Draw Two Triangules Without Using Vertex Indices

#### Question
1. Goal: Render two distinct triangles instead of one quad using vkCmdDraw().
2. Implementation:
	1. Locate the vertices vector in the code.
	2. Expand the vector to define 6 vertices, representing two separate triangles with unique positions and colours.
	3. Update the vertex buffer creation process to allocate enough memory for the 6 vertices.
	4. In recordCommandBuffer, replace the vkCmdDrawIndexed call with vkCmdDraw.
	5. Modify the vkCmdDraw call's vertex count from 4 to 6. You will not need an index buffer for this step.
3. Expected Outcome: Two separate colored triangles will be visible on the sreen.

#### Solution
At the start of the exercise, I had to do re-read the code to know exactly where to do the 
changes. At first I searched the part where the Quad vertices are created along with the 
indexes. Thus as seen on the next code below I changed them to be six due to the number of 
vertices on the two triangles.

```cpp
const std::vector<Vertex> Quad_vertices = {
    // First triangle (left)
    {{-0.8f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}}, // Bottom-left, Red
    {{-0.4f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}}, // Bottom-right, Green
    {{-0.6f,  0.2f, 0.0f}, {0.0f, 0.0f, 1.0f}}, // Top, Blue

    // Second triangle (right)
    {{ 0.4f, -0.5f, 0.0f}, {1.0f, 1.0f, 0.0f}}, // Bottom-left, Yellow
    {{ 0.8f, -0.5f, 0.0f}, {0.0f, 1.0f, 1.0f}}, // Bottom-right, Cyan
    {{ 0.6f,  0.2f, 0.0f}, {1.0f, 0.0f, 1.0f}}  // Top, Magenta
};
```

fter that I had to find the vertex buffer and allocate memory enough for the 6 vertices. 
Once I got that I went to find the vkCmdDrawIndexed and change it, I had to read the 
code inspector by hovering the mouse on the vkCmdDraw to know ehere to write the “6” 
value for the total vertices. As shown next.

```cpp
VkBuffer vertexBuffers[] = { vertexBuffer };
VkDeviceSize offsets[] = {0};
vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

// vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT16); // Not needed
vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[currentFrame], 0, nullptr);

vkCmdDraw(commandBuffer, 6, 1, 0, 0);
```

At the end I got the render going as seen the image below.

![Solution 1](<../markdown-resources/Complete Real Time Graphics Lab Book (1 - 8)/img1-1.png> "Rendering of the two triangles")

- Test data
N/A

- Sample output
Two triangles separated by some position with color rainbow (due to the vertex color) doing a yaw rotation.

- Reflection
    - *Reflect on what you have learnt from this exercise.*
      I learnt about changing the vertex vector in order to have more vertices and how to render them depending on the number of those.

    - *Did you make any mistakes?*
      No as far as I know. Although I struggles a bit on changing the `vkCmdDrawIndexed` due to not understanding it at first glance.

    - *In what way has your knowledge improved?*
      In the way that the two methods, ckCmdDraw and vkCmdDrawIndexed function. 
      And that depending on the number of vertices that need to be drawn you have to  change it in the function call.

- Questions

*Is there anything you would like to ask?*
No.

### Exercise 2: Draw Two Squares Using An Index Buffer

#### Question
1. Goal: Draw two squares (each composed of two triangles) using an index buffer to reuse vertices.
2. Implementation:
    1. Modify the vertices vector to define only the 4 unique corner vertices of a square.
    2. Create a new C++ vector for indices: const std::vector<uint16_t> indices = { ... ... }; (or a similar winding order). This defines two triangles that share vertices.
    3. Go through the code to understand how to create the VkBuffer for the index buffer(using the VK_BUFFER_USAGE_INDEX_BUFFER_BIT flag) and how to copy the index data to it using the staging buffer pattern.
    4. In recordCommandBuffer, replace the vkCmdDraw call with vkCmdDrawIndexed.
    5. Bind the index buffer using vkCmdBindIndexBuffer right before the draw call.
    6. The vkCmdDrawIndexed call should specify an index count of 6.
3. Expected Outcome: Two squares, identical in appearance to the original quad, but rendered more efficiently.

#### Solution
By creating a new vector<uint16_t> of vertices named Traingle_vertices, I had two 
vectors, the one for the quad and the one for the triangles. I had to learn to combine 
them in order to send them to the buffer in order to draw both meshes as seen on snippet below.
```cpp
const std::vector<Vertex> Quad_vertices = {
    {{-0.5f, -0.5f, 1.0f}, {1.0f, 0.0f, 0.0f}},
    {{0.5f, -0.5f, 1.0f}, {0.0f, 1.0f, 0.0f}},
    {{0.5f, 0.5f, 1.0f}, {0.0f, 0.0f, 1.0f}},
    {{-0.5f, 0.5f, 1.0f}, {1.0f, 1.0f, 1.0f}}
};

const std::vector<uint16_t> Quad_indices = {
    0, 1, 2, 2, 3, 0
};

const std::vector<Vertex> Triangle_vertices = {
    {{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}},
    {{0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}},
    {{0.5f, 0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}},
    {{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}}
};

const std::vector<uint16_t> Triangle_indices = {
    0, 1, 2 // First triangle
    2, 3, 0 // Second triangle
};

std::vector<Vector> vertices;
std::vector<uint16_t> indices;

void LoadModel() {
    // Combine vertices
    std::vector<Vertex> combinedVertices = Quad_vertices;
    combinedVertices.insert(combinedVertices.end(), Triangle_vertices.begin(), Triangle_vertices.end());

    // Combine indices, offsetting the triangle indices
    std::vector<uint16_t> combinedIndices = Quad_indices;
    uint16_t triangleVertexOffset = static_cast<uint16_t>(Quad_vertices.size());
    for (auto idx : Triangle_indices) {
        combinedIndices.push_back(idx + triangleVertexOffset);
    }

    vertices = combinedVertices;
    indices = combinedIndices;
}
```

By changing again to the function call of vckDrawIndexed I had to send the size of the 
indices vector
```cpp
vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
```

Lastly I got to render the two images as seen below.

![Solution 2](<../markdown-resources/Complete Real Time Graphics Lab Book (1 - 8)/img1-2.png> "Rendering the previous Quad")

- Test data
N/A

- Sample output
Two different meshes rendering at the same time, one is the quad colored in rainbow 
and a mesh formed by two triangles colored in red. I pushed the quad more upwards in 
order to see both of them.

- Reflection
*Reflect on what you have learnt from this exercise.*
I learned about joining vectors, in order to have two vectors a sending them to the 
buffer. And to call sizeof(indices) to get the length of the vector and passing it to the 
Draw call.

*Did you make any mistakes?*
Yes, I stumbled a lot until I got the join the vectors because it was just rendering 
the last one if I tried to pass the two vectors. Just the first one was rendering.

*In what way has your knowledge improved?*
In learning more techniques fin order to solve problems about the Vulkan 
pipeline.

- Questions

*Is there anything you would like to ask?*
No.

### Exercise 3: Draw The Four Walls Of A Cube

#### Question
1. Goal: Extend the previous exercise to draw the four side faces of a cube. 
2. Conceptual Overview: To see all faces of the cube correctly as it rotates, you 
may need to disable back-face culling. Culling is an optimization that discards 
triangles that are not facing the camera. By disabling it, we ensure that both 
front-facing and back-facing triangles are rendered.
3. Implementation:
    1. Define the 8 vertices of a cube.
    2. Define the indices for the 4 side faces. This will be 8 triangles, requiring 24 indices (4 faces * 2 triangles/face * 3 indices/triangle).
    3. Update the vertex and index buffers with this new data.
    4. Update the vkCmdDrawIndexed call to draw 24 indices.
    5. Locate the VkPipelineRasterizationStateCreateInfo struct used when creating the graphics pipeline.
    6. Set its cullMode to VK_CULL_MODE_NONE.
    7. You will need to recreate the graphics pipeline for this change to take effect.
4. Expected Outcome: The four side walls of a cube, rotating in space. You will be 
able to see through the open top and bottom. 

#### Solution
Firstly I had to create a new couple of vectors, one for the vertices of the cube and one 
fot its indices.

```cpp
const std::vector<Vertex> Cube_vertices = {
    // Front face (z = 0.5f)
    {{-0.5f, -0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}}, // 0: LB front, Red
    {{ 0.5f, -0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}}, // 1: RB front, Red
    {{ 0.5f,  0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}}, // 2: RT front, Red
    {{-0.5f,  0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}}, // 3: LT front, Red
    // Back face (z = -0.5f)
    {{-0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}}, // 4: LB back, Blue
    {{ 0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}}, // 5: RB back, Blue
    {{ 0.5f,  0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}}, // 6: RT back, Blue
    {{-0.5f,  0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}}  // 7: LT back, Blue
};

const std::vector<uint16_t> Cube_indices = {
    0, 1, 2, 2, 3, 0, // Front
    1, 5, 6, 6, 2, 1, // Right
    5, 4, 7, 7, 6, 5, // Back
    4, 0, 3, 3, 7, 4, // Left
    3, 2, 6, 6, 7, 3, // Top
    4, 5, 1, 1, 0, 4  // Bottom
};
```

After that, I searched for the rasterizer, and in its cull mode I changed it to none, this 
means that it will render both faces of the mesh.
```cpp
rasterizer.cullMode = VK_CULL_MODE_NONE;
```

When I finally run the project, a cube was appearing on the screen as seen  below.

![Solution 3](<../markdown-resources/Complete Real Time Graphics Lab Book (1 - 8)/img1-3.png> "Rendering the cube")

- Test data
N/A

- Sample output
A cube.

- Reflection
    - *Reflect on what you have learnt from this exercise.*
      I learned cull modes and how to interpret the vertices of a cube and the order of the faces in order to create it

    - *Did you make any mistakes?*
      Yes, I stumbled a little while forming the cube and on the culling modes.

    - *In what way has your knowledge improved?*  
      In learning about how to make a cube mesh

- Questions

    - *Is there anything you would like to ask?*
      Yes, I tried to render it with culling and one face was always black. Why did this 
      happen? I tried to fix it but couldn't, at the end I changed the cull mode to none

### Exercise 4: Wireframe Rendering

#### Question
1. Goal: Render the cube from the previous exercise as a wireframe, showing only 
its edges.
2. Implementation:
    1. In the VkPipelineRasterizationStateCreateInfo struct, change the polygonMode from `VK_POLYGON_MODE_FILL` to `VK_POLYGON_MODE_LINE`.
    2. Recreate the graphics pipeline and run the application.
3. Expected Outcome: A wireframe cube, where only the edges of the triangles 
are drawn.

#### Solution
So this one was fairly simple, just had to change the rasterizer’s polygon mode to 
`VK_POLYGON_MODE_LINE` to change tit to render its faces to line
```cpp
rasterizer.polygonMode = VK_POLYGON_MODE_LINE;
```

After the change it was a matter of running the program.

![Solution 4](<../markdown-resources/Complete Real Time Graphics Lab Book (1 - 8)/img1-4.png> "Rendering the cube’s wireframe")

- Test data
N/A

- Sample output
A cube’s wireframes

- Reflection
*Reflect on what you have learnt from this exercise.*
I learned how to change that polygon mode in the rasterizer.

*Did you make any mistakes?*
Yes, in how to write and call the pipeline function. 

*In what way has your knowledge improved?*
In learning about how to change the polygon mode.

- Questions

*Is there anything you would like to ask?*
Yes, I tried to render it with culling and one face was always black. Why did this 
happen? I tried to fix it but couldn't, at the end I changed the cull mode to none

### Exercise 5: Render The Cube's Vertices As Points

#### Question
1. Goal: Render only the eight vertices of the cube as individual points.
2. Implementation:
    1. Create a new graphics pipeline. In its`VkPipelineInputAssemblyStateCreateInfo`, set the topology member to `VK_PRIMITIVE_TOPOLOGY_POINT_LIST`.
    2. In your command buffer, bind this new pipeline.
    3. Use vkCmdDraw(commandBuffer, 8, 1, 0, 0); to draw the 8 vertices directly from the vertex buffer (no index buffer needed for this).
3. Expected Outcome: Eight distinct points rendered in the positions of the cube's vertices.

#### Solution
I had to create a new Graphics pipeline to have multiple functions. Although I had
problems applying the changes I got it to run. So, I copy the createGraphicsPipeline
function an created a new one.
```cpp
void HelloTriangleApplication::createGraphicsPipeline() {
    auto vertShaderCode = readFile("shaders/vert.spv");
    auto fragShaderCode = readFile("shaders/frag.spv");

    VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
    VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

    auto bindingDescription = Vertex::getBindingDescription();
    auto attributeDescriptions = Vertex::getAttributeDescriptions();

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    // ...
}
```

In that new function I modified the topology on the `createNewGraphicsPipeline` to
`VK_PRIMITIVE_TOPOLOGY_POINT_LIST`. In order to only render the points of the
vertices.
```cpp
void HelloTriangleApplication::createNewGraphicsPipeline() {
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;  // <- Change here
    // ...
}
```

I then Added the new function to the initialization calls, and had to comment the
previous one if not it didn’t render properly as seen in the snippet below.
```cpp
// --- Vulkan Initialization ---
void createInstance();
void setupDebugMessenger();
void createSurface();
void pickPhysicalDevice();
void createLogicalDevice();
void createSwapChain();
void createImageViews();
void createDescriptorSetLayout();
// createGraphicsPipeline(); // <- Commented out
void createNewGraphicsPipeline(); // <- New pipeline creation
void createCommandPool();
```

Lastly, I changed again to the call of ckCmdDraw and alter the parameters to 8 due to
the cube’s vertices.

```cpp
vkCmdDraw(commandBuffer, 8, 1, 0, 0);
```

And then I ran the program to obtain the render of the vertices points.

![Solution 5](<../markdown-resources/Complete Real Time Graphics Lab Book (1 - 8)/img1-5.png> "A cube vertices render")

- Test data
N/A

- Sample output
A cube’s vertices points being rendered

- Reflection
    - *Reflect on what you have learnt from this exercise.*
      I learned how to write the code for the rendering and have two functions, one for the
      faces and another one for the vertices. And how to change that topology in the input
      assembly.

    - *Did you make any mistakes?*
      Yes, in how to write and call the pipeline function.

*In what way has your knowledge improved?*
In which parts compose the graphics pipeline functions.

- Questions

    - *Is there anything you would like to ask?* 
      Yes, why when both graphics pipeline functions are called it doesn’t want to
      work? There’s an error on the build.

### Exercise 6: Render The Cube's Edges As Lines

#### Question
1. Goal: Render the 12 edges of the cube using line segments.
2. Implementation: Create a new index buffer containing indices for the 12 edges of the cube. Each edge requires 2 indices, for a total of 24 indices.
    1. Create another new graphics pipeline. This time, set the topology in `VkPipelineInputAssemblyStateCreateInfo` to `VK_PRIMITIVE_TOPOLOGY_LINE_LIST`.
    2. In the command buffer, bind the new pipeline and the new edge index buffer.
    3. Use vkCmdDrawIndexed(commandBuffer, 24, 1, 0, 0, 0); to draw the edges.
3. Expected Outcome: A wireframe cube, constructed from 12 individual line segments.

#### Solution
First on the createNewGraphicsPipeline I had to change the topology again, as seen on
the snippet below.
```cpp
void HelloTriangleApplication::createNewGraphicsPipeline() {
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;  // <- Change here
    // ...
}
```

Then I had to think about how to render the deges of the cube and wrote a new vector
to see them
```cpp
const std::vector<uint16_t> Cube_edge_indices = {
    0, 1, 1, 5, 5, 4, 4, 0, // Bottom
    3, 2, 2, 6, 6, 7, 7, 3, // Top
    0, 3, 1, 2, 5, 6, 4, 7  // Sides
};
```

And then, I had to change again the call from vkCmdDraw to the indexed one. I had to
write the number 24 due to the indices, but I could have used the indices.size().
```cpp
vkCmdDrawIndexed(commandBuffer, 24, 1, 0, 0, 0);
```

Lastly, I ran the program

![Solution 6](<../markdown-resources/Complete Real Time Graphics Lab Book (1 - 8)/img1-6.png> "A cube edges render")

- Test data
N/A

- Sample output
A cube’s edges being rendered

- Reflection
    - *Reflect on what you have learnt from this exercise.*
      I learned how to create the connection of the vertices in order to render edges.

    - *Did you make any mistakes?*
      No.

    - *In what way has your knowledge improved?*
      In practicing and thinking on how to connect the vertices to form the edges

- Questions

    - *Is there anything you would like to ask?*
      No.

### Exercise 7: Triangle Strips

#### Question
1. Goal: Render the cube using the `VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP` topology.
2. Conceptual Overview: A triangle strip is a more efficient way to render a series
of connected triangles. After the first three vertices form the first triangle, each
subsequent vertex creates a new triangle by combining with the previous two
vertices. To connect disjoint faces of the cube (like the sides), you may need to
insert "degenerate triangles"— zero-area triangles that are not rendered but
allow the GPU to restart the strip in a new location without breaking the primitive.
3. Implementation:
    1. Create a new index buffer with indices ordered to form a continuous strip covering the cube's faces. This will require careful planning and likely the
    use of degenerate triangles.
    2. Create a new graphics pipeline with the topology set to `VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP`.
    3. Bind the new pipeline and index buffer, and use vkCmdDrawIndexed with the correct index count.
4. Expected Outcome: A solid cube rendered using a single, efficient draw call.

#### Solution
Firstly, I had to think on how to do the strip, with helped of Copilot I got it to work, then I
changed the indices vector used in the program as a global variable with the new
indices.
```cpp
const std::vector<uint16_t> Cube_triangle_strips_indices = {
    0, 1, 3, 2, 6, 1, 5, 0, 4, 3, 7, 6, 4, 5, 5, 4, // Main strip
    5, 4,   // Degenerate: repeat last index of left face
    0, 1    // Bottom face
};

std::vector<Vertex> vertices;
std::vector<uint16_t> indices;

void loadModel() {
    //Combine vertices
    // std::vector<Vertex> combinedVertices = Quad_vertices;
    // combinedVertices.insert(combinedVertices.end(), Triangle_vertices.begin(), Triangle_vertices.end());

    // Combine indices, offsetting the triangle indices
    // std::vector<uint16_t> combinedIndices = Quad_indices;
    // uint16_t triangleVertexOffset = static_cast<uint16_t>(Quad_vertices.size());
    // for (auto idx : Triangle_indices) {
    //     combinedIndices.push_back(idx + triangleVertexOffset);
    // }

    vertices = Cube_vertices;
    indices = Cube_triangle_strips_indices;
}
```

Then with was a matter of following the instructions and change the inputAssembly
topology and the `vkCmdDrawIndexed`.
```cpp
void HelloTriangleApplication::createNewGraphicsPipeline() {
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;  // <- Change here
    // ...
}
```

```cpp
// vkCmdDraw(commandBuffer, 8, 1, 0, 0); // Previous call
// vkCmdDrawIndexed(commandBuffer, 24, 1, 0, 0, 0);  // Previous call
vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
```

Ath the end, the final result was a cube made with triangle strip connecting each index
with the following in order to create the cube as seen below.

![Solution 7](<../markdown-resources/Complete Real Time Graphics Lab Book (1 - 8)/img1-7.png> "A cube rendered with triangle strip")

- Test data
N/A

- Sample output
A cube rendered by only a single triangle strip

- Reflection
    - *Reflect on what you have learnt from this exercise.*
      I learned what degeneration is due to have to use it in order to fully render the cube

    - *Did you make any mistakes?*
      Yes, It was kind of hard thinking how to create the single line, I had to use the help of copilot and it explained me how to do it

    - *In what way has your knowledge improved?*
      In practicing and thinking on how to make a single strip triangle cube.

- Questions

    - *Is there anything you would like to ask?*
      No.

### Exercise 8: Drawing Multiple Cubes Using Instanced Drawing

#### Question
A more advanced and efficient method for drawing many copies of the same mesh is
"instancing".
This involves a single draw call that tells the GPU to render the object N times.
- **How it works**: You would issue one draw call: `vkCmdDrawIndexed(..., indexCount, instanceCount, ...);`, where instanceCount would be the number of
cube you would like to draw.
- **Shader Modification**: The vertex shader can then use the built-in
gl_InstanceIndex variable (which will be 0 for the first cube, 1 for the second, ... )
to calculate a unique position.
```glsl
// ... inside main() ...
vec3 offset = vec3(gl_InstanceIndex * 3.0 - 1.5, 0.0, 0.0);
mat4 instanceModel = ubo.model * translate(mat4(1.0), offset);
gl_Position = ubo.proj * ubo.view * instanceModel * vec4(inPosition, 1.0);
```
- **Advantages**: This significantly reduces CPU overhead as you only issue one
draw call and don't need to update the UBO between each instance. It is the
preferred method when rendering hundreds or thousands of identical objects.

#### Solution
This one was mainly about changing the vertex shader, as a first interaction with it, it
was not that complicated. We just had to find the code and make the necessary
changes as seen on on the shader snippet below.
```cpp
// vkCmdDraw(commandBuffer, 8, 1, 0, 0); // Previous call
// vkCmdDrawIndexed(commandBuffer, 24, 1, 0, 0, 0);  // Previous call
// vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);  // Previous call
vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(indices.size()), 5, 0, 0, 0);
```

```glsl
#version 450
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;

layout(binding = 0) uniform UBO {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

layout(location = 0) out vec3 fragColor;

void main() {
    fragColor = inColor;
    vec3 offset = vec3(gl_InstanceIndex * 3.0 - 5.0, 0.0, 0.0);
    mat4 instanceModel = ubo.model * mat4(1.0);
    instanceModel[3].xyz += offset;
    gl_Position = ubo.proj * ubo.view * instanceModel * vec4(inPosition, 1.0);
}
```

At the end I had to create 5 copies and modified the offset in order to render them, at
least 3

![Solution 8](<../markdown-resources/Complete Real Time Graphics Lab Book (1 - 8)/img1-8.png> "Multiple cubes rendered with instancing")

- Test data
N/A

- Sample output
Multiple cubes moved by an offset on the vertex shader.

- Reflection
    - *Reflect on what you have learnt from this exercise.*
      I learned how to modify the vertex shader in order to create multiple cubes moved by an offset.

    - *Did you make any mistakes?*
      No, although at the start I was lost in the solution explorer searching for the
      vertex shader.

    - *In what way has your knowledge improved?*
     In expanding my tools now by modifying the vertex directly, instead of only the C++ code of defining the mesh.

- Questions

    - *Is there anything you would like to ask?*
      No.

### Exercise 9: Drawing Two Cubes Using Push Constants

#### Question
1. Goal: Render two distinct wireframe cubes side-by-side using the same vertex/index buffers, but with different cube positions.
**Conceptual Overview**: A common task is to render the same object multiple
times at different locations. Replicating vertex data is inefficient. Instead, we
reuse the same mesh and provide different transformation data for each
"instance." The most efficient method is to use Push Constants. Push constants
allow you to send a small, limited amount of data to your shaders very quickly
during command buffer recording. This is perfect for data that changes with each
draw call, like an object's model matrix, without the overhead of managing
multiple descriptor sets.

2. Implementation:
    1. Update Your Vertex Shader: First, you need to modify your vertex shader to receive the model matrix as a push constant instead of from the
    Uniform Buffer Object (UBO). The view and proj matrices, which are the same for all objects in the frame, will remain in the UBO.
    2. In the main( ) method of your vertex shader, use the push constant bufferdata to reposition the cube:
            i. `gl_Position = ubo.proj * ubo.view * pushConstants.model * vec4(inPosition, 1.0);`
    3. Adjust Your C++ Structs.
    4. Modify the Pipeline Layout: In the method createGraphicsPipeline(), modify the graphics pipeline to inform Vulkan about the push constant
    range you intend to use when creating the pipeline layout.
    5. Update the Draw Command: Finally, in the `recordCommandBuffer()` method, on command buffer recording, you can now issue two draw calls.
    Before each one, you "push" a different model matrix to the shader.
3. Expected Outcome: Two identical wireframe cubes are rendered side-by-side.
Each draw call correctly uses its own unique model matrix because it's bound via
a separate descriptor set.

#### Solution
For this one I had to start with the vertex shader and as the instructions said I had to
separate the model as a PushConstants.
```glsl
#version 450
layout(binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
} ubo;

layout(push_constant) uniform PushConstants {
    mat4 model;
} pushConstants;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 0) out vec3 fragColor;

void main() {
    gl_Position = ubo.proj * ubo.view * pushConstants.model * vec4(inPosition, 1.0);
    fragColor = inColor;
}
```

Then change the struct of the UniformBufferObject and create on for the for the new
uniform, the ModelPushConstants
```cpp
struct UniformBufferObject {
    glm::mat4 view;
    glm::mat4 proj;
};

struct ModelPushConstants {
    glm::mat4 model;
};
```

Due to the separation, I had to comment the section that assigns the model to the
UniformBufferObject.
```cpp
UniformBufferObject ubo{};
ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
ubo.proj = glm::perspective(glm::radians(45.0f), swapChainExtent.width / (float) swapChainExtent.height, 0.1f, 10.0f);
ubo.proj[1][1] *= -1; // Invert Y for Vulkan
```

And then draw both of them, here I had to comment one ModelPushConstants due to
the errors on the output.
```cpp
// Draw first cube
ModelPushConstants pushUBO{};

pushUBO.model = glm::translate(glm::mat4(1.0f), glm::vec3(-1.0f, 0.0f, 0.0f));
vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(ModelPushConstants), &pushUBO);
vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);

// Draw second cube
pushUBO.model = glm::translate(glm::mat4(1.0f), glm::vec3(1.0f, 0.0f, 0.0f));
vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(ModelPushConstants), &pushUBO);

vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
```

And then finally, make the other changes requested by the instructions.
```cpp
VkPushConstantRange pushConstantRange{};
pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
pushConstantRange.offset = 0;
pushConstantRange.size = sizeof(ModelPushConstants);

pipelineLayoutInfo.pushConstantRangeCount = 1;
pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
```

When running the program two cubes appeared, static due to commenting the
ubf.model assignment previously.

![Solution 9](<../markdown-resources/Complete Real Time Graphics Lab Book (1 - 8)/img1-9.png> "Two cubes rendered with push constants")

- Test data
N/A

- Sample output
Two cubes rendered side by side.

- Reflection
    - *Reflect on what you have learnt from this exercise.*
      I learned about rendering multiple cubes with the same vertices and indices, and about push constants that are defined as
      “A small bank of values writable via the API and accessible in shaders. Push constants allow the application to set values used in shaders without creating buffers or
      modifying and binding descriptor sets for each update”.

    - *Did you make any mistakes?*
      Yes, thanks to reading the output I figured out how to properly find those errors.

    - *In what way has your knowledge improved?*
      In going to the deep end with this one I had to improve my tactics because there were multiple errors, I had to read the output and understand where the errors were
      coming from and going one by one until finding those

- Questions

    - *Is there anything you would like to ask?*
      Yes, I’d suggest having clearer instructions I found some details in the
      instructions for example in naming some values, one was plural and the other not and it
      brought errors. Just a friendly advice.

### Final Reflection
Thanks to this exercise I gathered new information on how the Vulkan pipeline is and
works. At first it was really intimidating, and it looked like a bunch of never-ending lines
of code. But it is a well-structured pipeline.

And although it is a bit complicated, it is well planned out and it gives you the ability to
render whatever you want. It is important to know where and what you are tinkering
with. And where to look out for if you want to change something.

---

## Lab book Chapter 2: Advanced geometric modelling

This session moves beyond hardcoded vertex data. You will learn how to generate complex geometric meshes procedurally—directly in your C++ code—to create shapes like grids,
terrains, and cylinders.
Furthermore, you will learn how to load 3D models from external files using Assimp, a powerful open-source asset importing library. These skills are fundamental for creating
dynamic and complex 3D scenes.

### Exercise 1: CREATE A FLAT GRID

#### Question
1. Goal: Generate the vertices and indices for a flat grid of arbitrary width and depth, centred at the origin, and render it in wireframe.
2. Implementation:
    1. Create a new C++ function, for example, createGrid(int width, int depth, `std::vector<Vertex>& outVertices`, `std::vector<uint32_t>& outIndices)`.
    2. Inside this function, use nested loops to generate the grid vertices.
    3. After generating the vertices, use another set of nested loops to generate the indices for the triangles that form the grid. The index list depends on the way
    the vertices are used to form triangles. For example, if it is based on triangle list, for each quad in the grid, you will need to generate two triangles (6
    indices). However, in terms of efficiency, you should consider using triangle strips.
    4. In your main application code, call this function to get the vertex and index data.
    5. Create the vertex buffer and the index buffer using the same staging buffer pattern from Lab 1.
    6. Modify your command buffer recording to bind the new vertex and index buffers and use `vkCmdDrawIndexed` with the correct index count for the grid.
    7. Ensure your pipeline is configured for wireframe rendering (`VK_POLYGON_MODE_LINE`).
3. Expected Outcome: A flat, wireframe grid rendered on the screen.

#### Solution
For this one I had to read all the previous lab work, until exercise 7 for me to get the
development environment ready for development. With the help of copilot I got the 
structure of the function and added my own bits and comments following the instructions
about the for loops
```cpp
void HelloTriangleApplication::createGrid(int width, int depth, std::vector<Vertex>& outVertices, std::vector<uint32_t>& outIndices) {
    outVertices.clear();
    outIndices.clear();

    // Generate vertices
    for (int z = 0; z <= depth; ++z) {
        for (int x = 0; x <= width; ++x) {
            float fx = static_cast<float>(x) - width / 2.0f;
            float fz = static_cast<float>(z) - depth / 2.0f;
            
            // Explicitly assign to glm::vec3 using x, y, z for clarity
            outVertices.push_back(Vertex{
                glm::vec3(fx, fz, 0.0f),
                glm::vec3(fx / width + 0.5f, fz / depth + 0.5f, 1.0f)
            });
        }
    }

    // Generate Indices for triangle strips with primitive restart
    const uint32_t RESTART_INDEX = 0xFFFFFFFF;
    for (int z = 0; z < depth; ++z) {
        for (int x = 0; x <= width; ++x) {
            outIndices.push_back(z * (width + 1) + x);
            outIndices.push_back((z + 1) * (width + 1) + x);
        }
        // Insert primitive restart index between strips (except after last strip)
        if (z < depth - 1) {
            outIndices.push_back(RESTART_INDEX);
        }
    }
}
```

In the initVulkan function I commented on the loadModel function and pretty much replace
it with the createGrid one.
```cpp
//LoadModel();
createGrid(10, 10, vertices, indices);
```

Of course, I had to add the declaration of the function in the class.
```cpp
void createGrid(int width, int depth, std::vector<Vertex>& outVertices, std::vector<uint32_t>& outIndices);
```

The changes done in the following code snippets are some of the things I had to change with
the help of the exercises in the Lab 1 workbook.
```cpp
inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
inputAssembly.primitiveRestartEnable = VK_TRUE;

rasterizer.polygonMode = VK_POLYGON_MODE_LINE;
rasterizer.lineWidth = 1.0f;
rasterizer.cullMode = VK_CULL_MODE_NONE;
```

Finally, due to some visual representation I didn’t like how it was appearing on the window,
the final render that is so I learned to modify the glm::lookAt function as it appears on the snippet below.
```cpp
UniformBufferObject ubo{};
//ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
ubo.model = glm::mat4(1.0f);
ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 5.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
ubo.proj = glm::perspective(glm::radians(45.0f), swapChainExtent.width / (float)swapChainExtent.height, 0.1f, 10.0f);
ubo.proj[1][1] *= -1;
```

The final rendering I was satisfied with.

![Solution 1](<../markdown-resources/Complete Real Time Graphics Lab Book (1 - 8)/img2-1.png> "Rendering the grid")

- Test data
N/A

- Sample output
A render of a Grid with a function in which you can modify its width and depth

- Reflection
    - *Reflect on what you have learnt from this exercise.*
      I learnt about the proper use of the triangle strips, and how to build more complex figures like a grid with the help of for loops. The algorithm and the mathematical process in
      order to get the vertices and indices accordingly to a grid figure.

    - *Did you make any mistakes?*
      Yes, I was stuck a lot in this exercise due to the fact that I downloaded the starting project again, so the configurations weren’t ready for this exercise.

    - *In what way has your knowledge improved?*
      In how to create and modify functions that can create more complex figures and the proper use of triangle strips.

- Questions

    - *Is there anything you would like to ask?*
      No.

### Exercise 2: CREATE A WAVY TERRAIN

#### Question
1. Goal: Modify the grid generation logic to create a simple, wavy terrain.
2. Implementation:
    1. Modify your createGrid function or create a new createTerrain function.
    2. Inside the vertex generation loop, instead of setting the y coordinate(corresponding to the up-direction) to 0, calculate it using a mathematical
       function based on the x and z coordinates, for example, setting y using a simple function: `y = sin(x) * cos(z);`.
    3. But to model a more meaningful and realistic terrain, you may need to usemore advanced techniques, such as the function defined based Perlin noise generation techniques.
    4. The index generation logic remains the same.
    5. Update your buffers and draw calls to render the new terrain mesh.
3. Expected Outcome: A terrain-like triangle mesh in wireframe.

#### Solution
For this one I again had to change the glm::lookAt function due to how the terrain was being
rendered.
```cpp
UniformBufferObject ubo{};
ubo.model = glm::mat4(1.0f);
ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 5.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
ubo.proj = glm::perspective(glm::radians(45.0f), swapChainExtent.width / (float)swapChainExtent.height, 0.1f, 10.0f);
ubo.proj[1][1] *= -1;
```

But before that let’s talk about adding the noise, with wasn’t difficult thanks to the glm
library and how it already has the code.
```cpp
#include <glm/gtc/noise.hpp> // For Perlin noise
```

Of course, I had to declare the new function.
```cpp
void createTerrain(int width, int depth, std::vector<Vertex>& outVertices, std::vector<uint32_t>& outIndices);
```

And then add it to the initVulkan function, commenting the previous one, the createGrid
function.
```cpp
// createGrid(5, 5, vertices, indices); // Previous call
createTerrain(50, 50, vertices, indices);
```

The following is the algorithm of the createTerrain function, the main difference between
this one and the previous one is the fact that you now have to control the altitude by
modifying the fy value of the vector3 by calling the perlin noise function.
```cpp
void HelloTriangleApplication::createTerrain(int width, int depth, std::vector<Vertex>& outVertices, std::vector<uint32_t>& outIndices) {
    outVertices.clear();
    outIndices.clear();

    // Generate vertices with Perlin noise height
    for (int z = 0; z <= depth; ++z) {
        for (int x = 0; x <= width; ++x) {
            float fx = static_cast<float>(x) - width / 2.0f;
            float fz = static_cast<float>(z) - depth / 2.0f;
            float scale = 0.2f; // Controls frequency of noise
            float fy = glm::perlin(glm::vec2(fx * scale, fz * scale)) * 2.0f; // Controls amplitude

            outVertices.push_back(Vertex{
                glm::vec3(fx, fy, fz),
                glm::vec3(fx / width + 0.5f, fy, fz / depth + 0.5f)
            });
        }
    }

    // Generate indices for triangle strips with primitive restart
    const uint32_t RESTART_INDEX = 0xFFFFFFFF;
    for (int z = 0; z < depth; ++z) {
        for (int x = 0; x <= width; ++x) {
            outIndices.push_back(z * (width + 1) + x);
            outIndices.push_back((z + 1) * (width + 1) + x);
        }
        if (z < depth - 1) {
            outIndices.push_back(RESTART_INDEX);
        }
    }
}
```

The final render is how it appears below.

![Solution 2](<../markdown-resources/Complete Real Time Graphics Lab Book (1 - 8)/img2-2.png> "Rendering the terrain")

- Test data
N/A

- Sample output
A render of a terrain with altitude depending on the Perlin noise.

- Reflection
    - *Reflect on what you have learnt from this exercise.*
      About the how to call the perlin noise and the many things that are available on the glm
      library. Of course, about the manipulation of the Y value on the vectors in order to represent altitude.

    - *Did you make any mistakes?*
      No.

    - *In what way has your knowledge improved?*
      In that now I know more about the algorithm needed in order to generate the grid and the manipulation of the altitude via the perlin function. If I can add a height map it
      would be the same logic.

- Questions

    - *Is there anything you would like to ask?*
      No.

### Exercise 3: PROCEDURAL CYLINDER

#### Question
1. Goal: Procedurally generate and render a cylinder mesh.
2. Conceptual Overview: A cylinder can be constructed from three parts: a top cap (a circle), a bottom cap (a circle), and the side walls. The side walls can be efficiently
created using a triangle strip that connects the vertices of the top and bottom circles.
3. Implementation:
    1. Create a C++ function `createCylinder(...)`.
    2. Generate the vertices for the bottom and top circular caps by sampling
    points along a circle at a given radius. The formula is `x = R * cos(theta)` and `z = R * sin(theta)`, where theta goes from 0 to 2π. Don't forget the centre vertex for each cap.
    3. Generate the indices for the top and bottom caps, forming triangles that connect the outer vertices to the centre vertex.
    4. Generate the indices for the cylinder walls, connecting the vertices of the bottom circle to the corresponding vertices of the top circle.
    5. Combine all vertices and indices into single vectors.
    6. Create the Vulkan buffers and issue a draw call to render the cylinder.
4. Expected Outcome: A rendered 3D cylinder, which you can view as solid or wireframe.

#### Solution
Again, for this one I did not like how the final render was being represented on the window,
thus I changed the glm::lookAt function again. This is a recurring thig now.
```cpp
ubo.view = glm::lookAt(glm::vec3(3.0f, 5.0f, 3.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
ubo.proj = glm::perspective(glm::radians(90.0f), swapChainExtent.width / (float)swapChainExtent.height, 0.1f, 10.0f);
```

Just as in previous exercises, I did the following steps again by making a function
declaration, this time the parameters were the radius, the height and the segments of the
cylinder along with the vertices and indices.
```cpp
void createCylinder(float radius, float height, int segments, std::vector<Vertex>& outVertices, std::vector<uint32_t>& outIndices);

// In initVulkan
createCylinder(2.0f, 3.0f, 10, vertices, indices);
```

```cpp
void HelloTriangleApplication::createCylinder(float radius, float height, int segments, std::vector<Vertex>& outVertices, std::vector<uint32_t>& outIndices) {
    outVertices.clear();
    outIndices.clear();

    // Generate bottom and top circle vertices
    uint32_t bottomCenterIndex = 0;
    uint32_t topCenterIndex = 1;
    outVertices.push_back(Vertex{ glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f) }); // bottom center
    outVertices.push_back(Vertex{ glm::vec3(0.0f, height, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f) }); // top center

    for (int i = 0; i < segments; ++i) {
        float theta = 2.0f * glm::pi<float>() * i / segments;
        float x = radius * std::cos(theta);
        float z = radius * std::sin(theta);

        // Bottom circle
        outVertices.push_back(Vertex{ glm::vec3(x, 0.0f, z), glm::vec3(1.0f, 0.0f, 0.0f) });
        // Top circle
        outVertices.push_back(Vertex{ glm::vec3(x, height, z), glm::vec3(0.0f, 1.0f, 0.0f) });
    }

    // Indices for bottom cap
    for (int i = 0; i < segments; ++i) {
        uint32_t next = (i + 1) % segments;
        outIndices.push_back(bottomCenterIndex);
        outIndices.push_back(2 + i * 2);
        outIndices.push_back(2 + next * 2);
    }

    // Indices for top cap
    for (int i = 0; i < segments; ++i) {
        uint32_t next = (i + 1) % segments;
        outIndices.push_back(topCenterIndex);
        outIndices.push_back(3 + i * 2);
        outIndices.push_back(3 + next * 2);
    }

    // Indices for cylinder wall
    for (int i = 0; i < segments; ++i) {
        uint32_t b0 = 2 + i * 2;
        uint32_t t0 = b0 + 1;
        uint32_t b1 = 2 + ((i + 1) % segments) * 2;
        uint32_t t1 = b1 + 1;

        // First triangle
        outIndices.push_back(b0);
        outIndices.push_back(t0);
        outIndices.push_back(b1);
        // Second triangle
        outIndices.push_back(b1);
        outIndices.push_back(t0);
        outIndices.push_back(t1);
    }
}
```

Now as it appears on the Img 3.4 I had to create a new algorithm, with the help of corpilot I
ended up with the following code: createCylinder function declaration.
- First, we have to add the vertices for the center of the bottom and the top.
- Then in a for loop we must add the vertices of the bottom and top circles by multiplying the segment number in the array with 2PI (a radian) by the segment and diving that by the total of segments.
- Then the following two for loops are about the cap (the edges) inside the circles. One for the top and one for the bottom.
- Finally, it was time to build the triangle strip that conforms the wall of the cylinder.

At the end the final render is the one as it shows below.

![Solution 3](<../markdown-resources/Complete Real Time Graphics Lab Book (1 - 8)/img2-3.png> "Rendering the cylinder")

- Test data
N/A

- Sample output
A cylinder rendered with the createCylinder function.

- Reflection

    - *Reflect on what you have learnt from this exercise.*
      I learnt about algorithms for creating geometric figures, and how it must be planned and represented on the vertices and indices vectors.

    - *Did you make any mistakes?*
      Yes, at first the algorithm was not working and when it did, the camera was not rendering properly.

    - *In what way has your knowledge improved?*
      I improved my knowledge on the different interpretations on how to render 3D figures, and the use of mathematics is of the utmost importance.

- Questions

    - *Is there anything you would like to ask?*
      No.


### Exercise 4: CREATING A REUSABLE GEOMETRY UTILITY

#### Question
1. Goal: Refactor the procedural generation code into a reusable C++ class or namespace, similar to the GeometryGenerator provided at d3d12book/Chapter 7
Drawing in Direct3D Part II at master · d3dcoder/d3d12book using the procedural geometric models defined in `GeometryGenerator.h`, `GeometryGenerator.cpp`
2. Implementation:
    1. Create a new C++ class or a set of free functions similar to the way shown in d3d12book/Chapter 7 Drawing in Direct3D Part II/Shapes/ShapesApp.cpp at master · d3dcoder/d3d12book.
    2. Move your grid, terrain, and cylinder generation logic into static methods orfunctions within this utility. Have them return a struct or std::pair containing the vertex and index vectors.
    3. In your main application, use this utility to generate meshes for a grid, a cylinder, and a sphere (you can add a createSphere function as a challenge).
    4. Render multiple different shapes in the same scene.
3. Expected Outcome: A scene containing multiple procedurally generated objects (e.g., a sphere and a cylinder sitting on a grid), each positioned independently.

#### Solution
For this one I had to do a lot of stuff, It wasn’t as simply as it appeared. First in order to
render each mesh as a separate object in Vulkan, I needed to:
- Store each mesh's vertices and indices in its own buffer (not append to global
vectors, this caused the issue detailed in The following Errors section).
- Track the buffers and draw parameters for each mesh.
- Issue a separate draw call for each mesh in your command buffer.

- Here's a step-by-step approach:
1. Define a Mesh Buffer Structure: Addedd a struct to hold the buffers and draw info for
each mesh as it appears on Img 4.1
   ```cpp
    struct MeshBuffers {
        VkBuffer vertexBuffer;
        VkDeviceMemory vertexBufferMemory;
        VkBuffer indexBuffer;
        VkDeviceMemory indexBufferMemory;
        uint32_t indexCount;
    };
   ```

2. Store Meshes Separately: Replaced the global vertices and indices with a vector of MeshBuffers.
    ```cpp
    std::vector<MeshBuffers> meshBuffers;
    ```

3. Create Buffers for Each Mesh: In initVulkan, for each mesh, I created its own vertex and index buffer:
    ```cpp
    std::vector<GeometryUtils::MeshData> meshes = {
        GeometryUtils::CreateGrid(5, 5, glm::vec3(2.0f, 2.0f, -3.0f)),
        GeometryUtils::CreateTerrain(50, 50, glm::vec3(0.0f, -3.0f, 0.0f)),
        GeometryUtils::CreateCylinder(2.0f, 3.0f, 18, glm::vec3(0.0f, 0.0f, 0.0f)),
        GeometryUtils::CreateSphere(2.0f, 10, 10, glm::vec3(0.0f, 6.0f, 5.0f))
    };

    meshBuffers.clear();
    ```

    ```cpp
    for (const auto& mesh : meshes) {
        std::vector<Vertex> verts;
        for (const auto& gv : mesh.vertices)
            verts.push_back(Vertex{ gv.pos, gv.color });

        VkDeviceSize vSize = sizeof(Vertex) * verts.size();
        VkDeviceSize iSize = sizeof(uint32_t) * mesh.indices.size();

        MeshBuffers buffers{};
        createBuffer(vSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, buffers.vertexBuffer, buffers.vertexBufferMemory);

        void* vData;
        vkMapMemory(device, buffers.vertexBufferMemory, 0, vSize, 0, &vData);
        memcpy(vData, verts.data(), (size_t)vSize);
        vkUnmapMemory(device, buffers.vertexBufferMemory);

        createBuffer(iSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, buffers.indexBuffer, buffers.indexBufferMemory);

        void* iData;
        vkMapMemory(device, buffers.indexBufferMemory, 0, iSize, 0, &iData);
        memcpy(iData, mesh.indices.data(), (size_t)iSize);
        vkUnmapMemory(device, buffers.indexBufferMemory);

        buffers.indexCount = static_cast<uint32_t>(mesh.indices.size());
        meshBuffers.push_back(buffers);
    }
    ```

4. Draw Each Mesh Separately: In recordCommandBuffer, I needed to loop through meshBuffers and issue a draw for each
    ```cpp
    for (const auto& mesh : meshBuffers) {
        VkBuffer vertexBuffers[] = { mesh.vertexBuffer };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(commandBuffer, mesh.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexed(commandBuffer, mesh.indexCount, 1, 0, 0, 0);
    }
    ```

5. Cleanup: In the cleanup method, I destroy all the mesh buffers.
    ```cpp
    void HelloTriangleApplication::cleanup() {
        for (const auto& mesh : meshBuffers) {
            vkDestroyBuffer(device, mesh.vertexBuffer, nullptr);
            vkFreeMemory(device, mesh.vertexBufferMemory, nullptr);
            vkDestroyBuffer(device, mesh.indexBuffer, nullptr);
            vkFreeMemory(device, mesh.indexBufferMemory, nullptr);
        }
        meshBuffers.clear();
    }
    ```

The following images from Img 4.6 to Img 4.9 are the implementation of the previous
functions that constructed the figures, except for an added parameter to all of them: offset.
This value lets me move around the object to a certain point in the final rendering

```cpp
#pragma once
#include <vector>
#include <glm/glm.hpp>

struct GeometryVertex {
    glm::vec3 pos;
    glm::vec3 color;
};

namespace GeometryUtils {
    struct MeshData {
        std::vector<GeometryVertex> vertices;
        std::vector<uint32_t> indices;
    };

    MeshData CreateGrid(int width, int depth, const glm::vec3& offset);
    MeshData CreateTerrain(int width, int depth, const glm::vec3& offset);
    MeshData CreateCylinder(float radius, float height, int segments, const glm::vec3& offset);
    MeshData CreateSphere(float radius, int stacks, int slices, const glm::vec3& offset);
}
```

```cpp
void HelloTriangleApplication::createGrid(int width, int depth, std::vector<Vertex>& outVertices, std::vector<uint32_t>& outIndices) {
    outVertices.clear();
    outIndices.clear();

    // Generate vertices
    for (int z = 0; z <= depth; ++z) {
        for (int x = 0; x <= width; ++x) {
            float fx = static_cast<float>(x) - width / 2.0f;
            float fz = static_cast<float>(z) - depth / 2.0f;
            
            // Explicitly assign to glm::vec3 using x, y, z for clarity
            outVertices.push_back(Vertex{
                glm::vec3(fx, fz, 0.0f),
                glm::vec3(fx / width + 0.5f, fz / depth + 0.5f, 1.0f)
            });
        }
    }

    // Generate Indices for triangle strips with primitive restart
    const uint32_t RESTART_INDEX = 0xFFFFFFFF;
    for (int z = 0; z < depth; ++z) {
        for (int x = 0; x <= width; ++x) {
            outIndices.push_back(z * (width + 1) + x);
            outIndices.push_back((z + 1) * (width + 1) + x);
        }
        // Insert primitive restart index between strips (except after last strip)
        if (z < depth - 1) {
            outIndices.push_back(RESTART_INDEX);
        }
    }
}
```

```cpp
void HelloTriangleApplication::createTerrain(int width, int depth, std::vector<Vertex>& outVertices, std::vector<uint32_t>& outIndices) {
    outVertices.clear();
    outIndices.clear();

    // Generate vertices with Perlin noise height
    for (int z = 0; z <= depth; ++z) {
        for (int x = 0; x <= width; ++x) {
            float fx = static_cast<float>(x) - width / 2.0f;
            float fz = static_cast<float>(z) - depth / 2.0f;
            float scale = 0.2f; // Controls frequency of noise
            float fy = glm::perlin(glm::vec2(fx * scale, fz * scale)) * 2.0f; // Controls amplitude

            outVertices.push_back(Vertex{
                glm::vec3(fx, fy, fz),
                glm::vec3(fx / width + 0.5f, fy, fz / depth + 0.5f)
            });
        }
    }

    // Generate indices for triangle strips with primitive restart
    const uint32_t RESTART_INDEX = 0xFFFFFFFF;
    for (int z = 0; z < depth; ++z) {
        for (int x = 0; x <= width; ++x) {
            outIndices.push_back(z * (width + 1) + x);
            outIndices.push_back((z + 1) * (width + 1) + x);
        }
        if (z < depth - 1) {
            outIndices.push_back(RESTART_INDEX);
        }
    }
}
```

```cpp
void HelloTriangleApplication::createCylinder(float radius, float height, int segments, std::vector<Vertex>& outVertices, std::vector<uint32_t>& outIndices) {
    outVertices.clear();
    outIndices.clear();

    // Generate bottom and top circle vertices
    uint32_t bottomCenterIndex = 0;
    uint32_t topCenterIndex = 1;
    outVertices.push_back(Vertex{ glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f) }); // bottom center
    outVertices.push_back(Vertex{ glm::vec3(0.0f, height, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f) }); // top center

    for (int i = 0; i < segments; ++i) {
        float theta = 2.0f * glm::pi<float>() * i / segments;
        float x = radius * std::cos(theta);
        float z = radius * std::sin(theta);

        // Bottom circle
        outVertices.push_back(Vertex{ glm::vec3(x, 0.0f, z), glm::vec3(1.0f, 0.0f, 0.0f) });
        // Top circle
        outVertices.push_back(Vertex{ glm::vec3(x, height, z), glm::vec3(0.0f, 1.0f, 0.0f) });
    }

    // Indices for bottom cap
    for (int i = 0; i < segments; ++i) {
        uint32_t next = (i + 1) % segments;
        outIndices.push_back(bottomCenterIndex);
        outIndices.push_back(2 + i * 2);
        outIndices.push_back(2 + next * 2);
    }

    // Indices for top cap
    for (int i = 0; i < segments; ++i) {
        uint32_t next = (i + 1) % segments;
        outIndices.push_back(topCenterIndex);
        outIndices.push_back(3 + i * 2);
        outIndices.push_back(3 + next * 2);
    }

    // Indices for cylinder wall
    for (int i = 0; i < segments; ++i) {
        uint32_t b0 = 2 + i * 2;
        uint32_t t0 = b0 + 1;
        uint32_t b1 = 2 + ((i + 1) % segments) * 2;
        uint32_t t1 = b1 + 1;

        // First triangle
        outIndices.push_back(b0);
        outIndices.push_back(t0);
        outIndices.push_back(b1);
        // Second triangle
        outIndices.push_back(b1);
        outIndices.push_back(t0);
        outIndices.push_back(t1);
    }
}
```

For the sphere, I used copilot to help come up with a solution, by creating two for loops for
the vertices’ creation and another nested for loops for the indices and, again, an offset in
order to move to a specific point in the space the figure. I was able to render the sphere.
If I spread a sphere in a 2D plane, it would be a collection of lines, the horizontal ones are
called stacks and the vertical ones are called slices. This is the core concept of the for
loops in the generation of the sphere. The position of the vertices’ horizontal value depends
on the radius multiplied by the sin and cos of PI * the number of the stack divided by the
total of stacks. And the vertical value depends on the sin and cos on the radian (2PI) times
the number of the slice divided by the total of slices
```cpp
MeshData GeometryUtils::CreateSphere(float radius, int stacks, int slices, const glm::vec3& offset) {
    MeshData mesh;
    for (int i = 0; i <= stacks; ++i) {
        float phi = glm::pi<float>() * i / stacks;
        float y = radius * std::cos(phi);
        float r = radius * std::sin(phi);

        for (int j = 0; j <= slices; ++j) {
            float theta = 2.0f * glm::pi<float>() * j / slices;
            float x = r * std::cos(theta);
            float z = r * std::sin(theta);

            mesh.vertices.push_back(GeometryVertex{
                glm::vec3(x, y, z) + offset,
                glm::vec3((float)j / slices, (float)i / stacks, 1.0f)
            });
        }
    }

    for (int i = 0; i < stacks; ++i) {
        for (int j = 0; j < slices; ++j) {
            int first = i * (slices + 1) + j;
            int second = first + slices + 1;

            mesh.indices.push_back(first);
            mesh.indices.push_back(second);
            mesh.indices.push_back(first + 1);

            mesh.indices.push_back(second);
            mesh.indices.push_back(second + 1);
            mesh.indices.push_back(first + 1);
        }
    }
    return mesh;
}
```

Thanks to the offset value I can move around the figures and got the to render properly as it
appears on the image below.

![Solution 4](<../markdown-resources/Complete Real Time Graphics Lab Book (1 - 8)/img2-4.png> "Rendering multiple figures")

- Test data
N/A

- Sample output
A final render of each object separated and in complete control of its own position in the world space.

- Reflection

    - *Reflect on what you have learnt from this exercise.*
      I learnt about the rendering of a sphere, as well as the process of making a namespace and calling it on another script. This helps the code to be reusable and more easy to read.

    - *Did you make any mistakes?*
      Yes at the start my approach was different and instead of rendering as different objects, I annexed the vertices and indices, and all the figures converged on one vertex.
      ![Error on Solution 4](<../markdown-resources/Complete Real Time Graphics Lab Book (1 - 8)/img2-4-error.png> "Error by appending the vertices and indices")]

    - *In what way has your knowledge improved?*
      In that I learnt that In order to properly render multiple objects I have to change too the buffers, and have a draw call to each one of them as a collection instead of appending everything.

- Questions

    - *Is there anything you would like to ask?*
      No.

### Exercise 5: LOADING EXTERNAL MODELS WITH ASSIMP
Question
1. Goal: Integrate the Assimp library into your project to load and render a 3D model from an .obj file.
2. Implementation:
    1. Install Assimp: In Visual Studio, right-click your project in the Solution Explorer and select Manage NuGet Packages. Browse for assimp.native and install it. Recreate the graphics pipeline and run the applicationSolution
    2. Choose Browse and search “Assimp”, a list of Assimp libraries of different versions will be displayed, for example, Assimp_native_4.1.
    3. Include Headers: In the C++ file where you will load the model, include the necessary Assimp headers.
    4. Load the Model: Use the Assimp importer to read a model file (you can find simple .obj files like a teapot or a cube online).
    5. Process Mesh Data: The aiScene object contains one or more meshes. For a simple model, you can process the first one.
    6. Extract vertex data.
    7. Extract index data.
    8. Create Buffers and Draw: The vertices and indices vectors now contain your model data. Use this data to create your VkBuffers for the vertex and index buffers, and update your command buffer to draw the loaded model.
3. Expected Outcome: A wireframe cube, where only the edges of the triangles are drawn

#### Solution
For this exercise, I had to follow the instructions, but instead of downloading the one
showed, I downloaded the one suggested the picture below.

![Assimp NuGet](<../markdown-resources/Complete Real Time Graphics Lab Book (1 - 8)/img2-5-nuget.png> "Assimp NuGet package")

After downloading it, I created the namespace named MeshLoader where I created a function called LoadMeshFromFile in which, in the body, I adapted the return value to
return a MeshData from GeometryUtils created in the previous exercise.
```cpp
#pragma once
#include "GeometryUtils.h"

namespace MeshLoader {
    GeometryUtils::MeshData LoadMeshFromFile();
}
```

```cpp
#include "MeshLoader.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

GeometryUtils::MeshData MeshLoader::LoadMeshFromFile() {
    GeometryUtils::MeshData meshData;
    Assimp::Importer importer;

    const aiScene* scene = importer.ReadFile("C:\\Users\\949145\\Documents\\Teapot.obj", aiProcess_Triangulate | aiProcess_FlipUVs);
    aiMesh* mesh = scene->mMeshes[0];

    for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
        GeometryVertex vertex;
        vertex.pos = {
            mesh->mVertices[i].x,
            mesh->mVertices[i].y,
            mesh->mVertices[i].z
        };
        vertex.color = { 1.0f, 1.0f, 1.0f };
        meshData.vertices.push_back(vertex);
    }

    for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++) {
            meshData.indices.push_back(face.mIndices[j]);
        }
    }
    return meshData;
}
```

Added the header to the Lab_tutorial_Template in order to use the function
LoadMeshFromFile. As it appears in the Img 5.5 I had to comment the other function calls
for GeometryUtils.
```cpp
#include "GeometryUtils.h"
#include "MeshLoader.h"
```

```cpp
// In initVulkan Generate meshes section
std::vector<GeometryUtils::MeshData> meshes = {
    MeshLoader::LoadMeshFromFile()
};
```

To build and run the project I had to fix some problems where it didn’t read the .lib and the .dll files. For that I had to open the Project Properties and on the Linker I had to add the libraries and the path to the files.

![Linker Settings](<../markdown-resources/Complete Real Time Graphics Lab Book (1 - 8)/img2-5-linker.png> "Linker settings for Assimp")

![Input Settings](<../markdown-resources/Complete Real Time Graphics Lab Book (1 - 8)/img2-5-input.png> "Input settings for Assimp")]

To fix the last error copilot suggested to add the .dll in the same file where the .exe is

![DLL Location](<../markdown-resources/Complete Real Time Graphics Lab Book (1 - 8)/img2-5-dll.png> "Assimp .dll location")]

After that I got the project to run and render in the correct settings the .obj of a teapot that I downloaded

![Solution 5](<../markdown-resources/Complete Real Time Graphics Lab Book (1 - 8)/img2-5.png> "Rendering the teapot")

- Test data
A file named *Teapot.obj*.

- Sample output
A render of the teapot with the settings made in Vulkan.

- Reflection

    - *Reflect on what you have learnt from this exercise.*
      I learnt to use the NuGet properties of Visual Studio and use extensions in order to render .obj objects and modified them in the instructions made with the Vulkan pipeline.

    - *Did you make any mistakes?*
      Yes, I had to do quite the research in order to get the project to run, it was not that my code was wrong, but the properties settings of Visual Studio and the solution weren’t set correctly.
      ![Error on Solution 5](<../markdown-resources/Complete Real Time Graphics Lab Book (1 - 8)/img2-5-error.png> "Error due to .dll")]
    
    - *In what way has your knowledge improved?*
      It improved in the sense that I had to do research and investigate how to fix these issues, If I hadn’t followed the Vulkan Tutorial webpage maybe it would’ve been more
      difficult for me to fix. But now I understand what Linker does and how to change it.

- Questions

    - *Is there anything you would like to ask?*
      Yes, while I didn’t use the recommended extension and didn’t check if that one worked, I was wondering if there were any problems

### Final Reflection
This week, we worked on meshes algorithms and how to set them up. For loops, radians
and other mathematical techniques to get the vertices properly and the indices too.
Depending on the geometric figure these techniques and algorithms change but for basic
figures it has been well documented on how to use them.

Also, thanks to libraries and extensions we can render .obj objects and change the pipeline
through Vulkan in order to render them and change the shaders and graphic pipeline.


---

## Lab book Chapter 3: Transformation

In the previous labs, you learnt to create, load, and render static 3D meshes. This lab
introduces the fundamental concepts of 3D transformations, which allow us to position,
rotate, and scale those meshes to create a dynamic and coherent scene. You will learn how
to use matrices to manipulate objects in 3D space and understand the pipeline of
transformations that takes an object from its local coordinate system all the way to the 2D
screen. These concepts are the cornerstone of all 3D graphics and animation.

### Exercise 1: BASIC SCALING TRANSFORMATION

#### Question
1. Goal: Deform the cube you created from Lab 1 into a long, flat plank using a non- uniform scaling transformation.
2. Implementation:
    1. In your C++ code, before updating the UBO, create a scaling matrix using GLM. For example, `glm::mat4 modelMatrix = glm::scale(glm::mat4(1.0f)`, `glm::vec3(2.5f, 0.5f, 0.5f));`.
    2. Assign this modelMatrix to the appropriate member of your UBO struct.
    3. Update the UBO on the GPU and run the application.
3. Expected Outcome: Display the transformed cubes shown below

#### Solution
For this exercise, I had to change the polygon mode to VK_POLYGON_MODE_FILL in order to render the cubes faces and not only its edges as shown below.
```cpp
rasterizerInfo.polygonMode = VK_POLYGON_MODE_FILL;
```

Then I checked the cube creation that I did on Lab 1 and made it a function on GeometryUtils. Added it to the header and implemented the function on its body.
```cpp
MeshData CreateCube(float size, const glm::vec3& offset);
```

```cpp
Geometrytils::MeshData GeometryUtils::CreateCube(float size, const glm::vec3& offset) {
    MeshData mesh;
    float h = size * 0.5f;

    // Define 6 ace colors (RGB)
    glm::vec3 faceColors[6] = {
        glm::vec3(1.0f, 0.0f, 0.0f), // -X (red)
        glm::vec3(0.0f, 1.0f, 0.0f), // +X (green)
        glm::vec3(0.0f, 0.0f, 1.0f), // -Y (blue)
        glm::vec3(1.0f, 1.0f, 0.0f), // +Y (yellow)
        glm::vec3(1.0f, 0.0f, 1.0f), // -Z (magenta)
        glm::vec3(0.0f, 1.0f, 1.0f)  // +Z (cyan)
    };

    // Each face hhas 4 vertices (duplicated for flat color per face)
    glm::vec3 positions[24] = {
        // -X
        { {-h, -h, -h}, {-h, -h, h}, {-h, h, h}, {-h, h, -h} },
        // +X
        { {h, -h, h}, {h, -h, -h}, {h, h, -h}, {h, h, h} },
        // -Y
        { {-h, -h, h}, {-h, -h, -h}, {h, -h, -h}, {h, -h, h} },
        // +Y
        { {-h, h, -h}, {-h, h, h}, {h, h, h}, {h, h, -h} },
        // -Z
        { {h, -h, -h}, {-h, h, -h}, {-h, h, h}, {h, -h, h} },
        // +Z
        { {-h, -h, h}, {h, -h, h}, {h, h, h}, {-h, h, h} },
    };

    // Add vertices and colors
    for(int face = 0; face < 6; ++face) {
        for (int v = 0; v < 4; ++v) {
            mesh.vertices.push_back(GeometryVertex{
                positions[face][v] + offset,
                faceColors[face]
            });
        }
    }

    // Indices for 2 traingles per face (6 faces)
    for (int face = 0; face < 6; ++face) {
        uint32_t base = face * 4;
        mesh.indices.push_back(base + 0);
        mesh.indices.push_back(base + 1);
        mesh.indices.push_back(base + 2);
        mesh.indices.push_back(base + 2);
        mesh.indices.push_back(base + 3);
        mesh.indices.push_back(base + 0);
    }

    return mesh;
}
```

I commented out the other calls to GeometryUtils to only make the cube appear and test out the function as it appears on Img. 1.4 and got to render the cube although it appeared
with its faces not rendering that properly. Tried to change around the graphics pipeline but I didn’t get it to render that well, I’m guessing it was because of the triangle strip mode.

```cpp
// Generate meshes
std::vector<GeometryUtils::MeshData> meshes = {
    GeometryUtils::CreateCube(2.0f, glm::vec3(0.0f, 0.0f, 0.0f))
};
```

![Solution 1.1](<../markdown-resources/Complete Real Time Graphics Lab Book (1 - 8)/img3-1-cube.png> "Rendering the cube")

```cpp
UniformBufferObject ubo{};
glm::mat4 modelMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(2.5f, 0.5f, 0.5f));
ubo.model = modelMatrix;
ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
ubo.proj = glm::perspective(glm::radians(45.0f), swapChainExtent.width / (float)swapChainExtent.height, 0.1f, 10.0f);
ubo.proj[1][1] *= -1; // Invert Y for Vulkan
```

After that I went to the updateUniformBuffer function and changed the scale by creating a new mat4 named modelMatrix and assigned it to the ubo.model.

![Solution 1.2](<../markdown-resources/Complete Real Time Graphics Lab Book (1 - 8)/img3-1-pillar.png> "Rendering the pillar")

Then I just had to change the scale vec3 again to make it a floor object as it appears rendered below

```cpp
UniformBufferObject ubo{};
glm::mat4 modelMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(0.5f, 0.05f, 0.5f));
ubo.model = modelMatrix;
ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
ubo.proj = glm::perspective(glm::radians(45.0f), swapChainExtent.width / (float)swapChainExtent.height, 0.1f, 10.0f);
ubo.proj[1][1] *= -1; // Invert Y for Vulkan
```

![Solution 1.3](<../markdown-resources/Complete Real Time Graphics Lab Book (1 - 8)/img3-1-flat.png> "Rendering the flat")

- Test data
N/A

- Sample output
A cylinder rendered with the createCylinder function.

- Reflection

    - *Reflect on what you have learnt from this exercise.*
      I learnt about algorithms for creating geometric figures, and how it must be planned and represented on the vertices and indices vectors.

    - *Did you make any mistakes?*
      Yes, at first the algorithm was not working and when it did, the camera was not rendering properly.

    - *In what way has your knowledge improved?*
      I improved my knowledge on the different interpretations on how to render 3D figures, and the use of mathematics is of the utmost importance.

- Questions

    - *Is there anything you would like to ask?*
      No.

### Exercise 2: HIERARCHICAL TRANSFORMATIONS

#### Question
1. Goal: Apply scaling, translation, and rotation transformations to achieve the
following visual outcomes:
    1. Single Cube Rotation A cube rotates around a vertical axis, as illustrated in Figure (a).
    2. Dual Cube Rotation Two cubes rotate simultaneously, each around a distinct rotational axis and at different rotational speeds, as shown in Figure (b).
*Note: The rotational axes are represented by scaled versions of the cube model you created in Lab 1.*
2. Implementation
    1. For the pillars: Apply a scaling matrix to make it tall and thin and a translation matrix to place it at the world origin.
    2. For the rotating small cube: The transformation must be calculated each frame.
        1. Start with an identity matrix.
        2. Apply a rotation matrix that changes over time (e.g., based on a frame counter). This makes the moon orbit the origin.
        3. Apply a translation matrix after the rotation to push the rotating cube away from the origin.
        4. The combined matrix will be Translation * Rotation.
    3. Update the respective UBOs for the pillar and the moon and issue two separate draw calls.
3. Expected Outcome: The visual effects shown below

#### Solution
So, because this time around I must change multiple objects, I created a global variable of
type `vector<glm::mat4>` to store the model matrix for each one of the objects. After I called
the CreateCube functions, I had to create and save those models matrices later the
initVulkan function

```cpp
std::vector<glm::mat4> meshBuffers;

std::vector<glm::mat4> modelMatrices;
```

```cpp
// Generate meshes
std::vector<GeometryUtils::MeshData> meshes = {
    /*GeometryUtils::CreateCube(2.0f, glm::vec3(-3.0f, 0.0f, 0.0f)),
    GeometryUtils::CreateGrid(5, 5, glm::vec3(2.0f, 2.0f, -3.0f)),
    GeometryUtils::CreateTerrain(50, 50, 0.2f, glm::vec3(0.0f, -3.0f, 0.0f)),
    GeometryUtils::CreateCylinder(2.0f, 3.0f, 10, glm::vec3(0.0f, 0.0f, 0.0f)),
    GeometryUtils::CreateSphere(2.0f, 10, 10, glm::vec3(0.0f, 0.0f, 5.0f)),
    MeshLoader::LoadMeshFromFile("C:\\Users\\949145\\Documents\\Teapot.obj")*/
    GeometryUtils::CreateCube(2.0f, glm::vec3(0.0f, 0.0f, 0.0f)), // floor
    GeometryUtils::CreateCube(2.0f, glm::vec3(0.0f, 0.0f, 0.0f)), // pillar one
    GeometryUtils::CreateCube(2.0f, glm::vec3(0.0f, 0.0f, 0.0f))  // cube 1
};

// Clear global vectors
meshBuffers.clear();

// Example: two cubes, one plank, one normal
modelMatrices.clear();

modelMatrices.push_back(glm::scale(glm::mat4(1.0f), glm::vec3(5.0f, 0.05f, 5.0f)));    // floor scale
modelMatrices.push_back(glm::scale(glm::mat4(1.0f), glm::vec3(0.25f, 3.0f, 0.25f)));  // pillar one scale
modelMatrices.push_back(glm::scale(glm::mat4(1.0f), glm::vec3(0.5f, 0.5f, 0.5f)));    // cube 1 scale
// Add more for additional meshes as needed
```

Then I loop through the meshBuffer’s size to draw each of the objects with its model matrix
assigned by the one corresponding to the index of the current mesh buffer. This is tricky
because if the order is wrong, the intent of the solution fails.

```cpp
for (size_t i = 0; i < meshBuffers.size(); ++i) {
    VkBuffer vertexBuffers[] = { meshBuffers[i].vertexBuffer };
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(commandBuffer, meshBuffers[i].indexBuffer, 0, VK_INDEX_TYPE_UINT32);

    // Use the model matrix for this mesh
    ModelPushConstant pushUBO{};
    pushUBO.model = modelMatrices[i];
    vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(ModelPushConstant), &pushUBO);

    vkCmdDrawIndexed(commandBuffer, meshBuffers[i].indexCount, 1, 0, 0, 0);
}
```

```cpp
void HelloTriangleApplication::updateUniformBuffer(uint32_t currentImage) {
    static auto startTime = std::chrono::high_resolution_clock::now();
    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float>(currentTime - startTime).count();

    UniformBufferObject ubo{};
    //ubo.view = glm::lookAt(glm::vec3(2.0f, 5.0f, 5.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    ubo.view = glm::lookAt(
        glm::vec3(0.0f, 10.0f, 0.0f), // Camera position: high above the origin
        glm::vec3(0.0f, 0.0f, 0.0f),  // Look at the origin
        glm::vec3(0.0f, 0.0f, -1.0f)  // Up is negative Z (for a right-handed system)
    );
    ubo.proj = glm::perspective(glm::radians(45.0f), swapChainExtent.width / (float)swapChainExtent.height, 0.1f, 10.0f);
    ubo.proj[1][1] *= -1;

    // --- Exercise 2: Rotating cube around pillar ---
    if (modelMatrices.size() > 2) {
        glm::mat4 identity = glm::mat4(1.0f);
        glm::mat4 rotation = glm::rotate(identity, time, glm::vec3(0.0f, 1.0f, 0.0f)); // rotate around Y
        glm::mat4 translation = glm::translate(identity, glm::vec3(2.0f, 0.0f, 0.0f)); // move away from pillar
        glm::mat4 scale = glm::scale(identity, glm::vec3(0.5f, 0.5f, 0.5f));           // keep the cube small

        // Model = Rotation * Translation * Scale (orbit around pillar)
        modelMatrices[2] = rotation * translation * scale;
    }

    memcpy(uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
}
```

In the updateUniformBuffer function, I had to create changes because I needed the cube to
rotate around the y-axis of the pillar. I changed the model matrix rotation with the function
glm::rotate and the translation with glm::translate to move it away from the pillar. As it
shows on Img. 2.5. I got the cube to rotate along the y-axis of the pillar.

![Render of pillar, floor and cube](<../markdown-resources/Complete Real Time Graphics Lab Book (1 - 8)/img3-2-orbit.png> "Render of pillar, floor and cube")

```cpp
VkPushConstantRange pushConstantRange{};
pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
pushConstantRange.offset = 0;
pushConstantRange.size = sizeof(ModelPushConstant);

VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
pipelineLayoutInfo.setLayoutCount = 1;
pipelineLayoutInfo.pSetLayout = &descriptorSetLayout;
pipelineLayoutInfo.pushConstantRangeCount = 1;
pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
```
To make two objects appear base from the same object, in the graphics pipeline I had to
change it just like in lab 1. I had to add the pipelineLayoutInfo for the push constants, and
create the a `VkPushConstantRange`.

```glsl
#version 450

layout(push_constant) uniform PushConstants {
    mat4 model;
} pc;

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;

layout(location = 0) out vec3 fragColor;

void main() {
    gl_Position = ubo.proj * ubo.view * pc.model * vec4(inPosition, 1.0);
    fragColor = inColor;
}
```

Just like in Lab 1, I also had to separate the model and assign it with the push constants.
Then in the initVulkan I created another pillar and another cube and added their respective
matrices to modify the scale and position.

```cpp
// Generate meshes
std::vector<GeometryUtils::MeshData> meshes = {
    /*GeometryUtils::CreateCube(2.0f, glm::vec3(-3.0f, 0.0f, 0.0f)),
    GeometryUtils::CreateGrid(5, 5, glm::vec3(2.0f, 2.0f, -3.0f)),
    GeometryUtils::CreateTerrain(50, 50, 0.2f, glm::vec3(0.0f, -3.0f, 0.0f)),
    GeometryUtils::CreateCylinder(2.0f, 3.0f, 10, glm::vec3(0.0f, 0.0f, 0.0f)),
    GeometryUtils::CreateSphere(2.0f, 10, 10, glm::vec3(0.0f, 0.0f, 5.0f)),
    MeshLoader::LoadMeshFromfile("C:\\Users\\949145\\Documents\\Teapot.obj")*/
    GeometryUtils::CreateCube(2.0f, glm::vec3(0.0f, 0.0f, 0.0f)), // floor
    GeometryUtils::CreateCube(2.0f, glm::vec3(0.0f, 0.0f, 0.0f)), // pillar 1
    GeometryUtils::CreateCube(2.0f, glm::vec3(0.0f, 0.0f, 0.0f)), // cube 1
    GeometryUtils::CreateCube(2.0f, glm::vec3(0.0f, 0.0f, 0.0f)), // pillar 2
    GeometryUtils::CreateCube(2.0f, glm::vec3(0.0f, 0.0f, 0.0f))  // cube 2
};

// Clear global vectors
meshBuffers.clear();

// Example: two cubes, one plank, one normal
modelMatrices.clear();

modelMatrices.push_back(glm::scale(glm::mat4(1.0f), glm::vec3(5.0f, 0.05f, 5.0f))); // floor scale
modelMatrices.push_back(
    glm::translate(glm::mat4(1.0f), glm::vec3(-2.0f, 0.0f, 0.0f)) *
    glm::scale(glm::mat4(1.0f), glm::vec3(0.25f, 3.0f, 0.25f))
); // pillar 1 scale
modelMatrices.push_back(glm::scale(glm::mat4(1.0f), glm::vec3(0.5f, 0.5f, 0.5f))); // cube 1 scale
modelMatrices.push_back(
    glm::translate(glm::mat4(1.0f), glm::vec3(2.0f, 0.0f, 0.0f)) *
    glm::scale(glm::mat4(1.0f), glm::vec3(0.25f, 3.0f, 0.25f))
); // pillar 2 scale
modelMatrices.push_back(glm::scale(glm::mat4(1.0f), glm::vec3(0.5f, 0.5f, 0.5f))); // cube 2 scale
// Add more for additional meshes as needed
```

In order to have the objects positions and rotation we needed to also modify again the
updateUniformBuffer to change the matrices for the cubes so they can rotate, one
clockwise and the other anti-clockwise along each pillar’s y-axis. And assign them the
proper center of their rotation.

```cpp
void HelloTriangleApplication::updateUniformBuffer(uint32_t currentImage) {
    static auto startTime = std::chrono::high_resolution_clock::now();
    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float>(currentTime - startTime).count();

    UniformBufferObject ubo{};
    ubo.view = glm::lookAt(glm::vec3(3.0f, 8.0f, 8.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    /*ubo.view = glm::lookAt(
        glm::vec3(0.0f, 15.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, -1.0f)
    );*/
    ubo.proj = glm::perspective(glm::radians(45.0f), swapChainExtent.width / (float)swapChainExtent.height, 0.1f, 50.0f);
    ubo.proj[1][1] *= -1;

    // Only update the cube (modelMatrices[2])
    if (modelMatrices.size() > 2) {
        glm::mat4 identity = glm::mat4(1.0f);

        // Pillar's world center (from initVulkan: -2,0,0)
        glm::vec3 pillarCenter(-2.0f, 0.0f, 0.0f);

        // Orbit radius (distance from pillar center)
        glm::vec3 orbitOffset(2.0f, 0.0f, 0.0f);

        // Build the model matrix: translate to pillar, rotate, translate out, scale
        modelMatrices[2] =
            glm::translate(identity, pillarCenter) *
            glm::rotate(identity, time, glm::vec3(0.0f, 1.0f, 0.0f)) *
            glm::translate(identity, orbitOffset) *
            glm::scale(identity, glm::vec3(0.5f, 0.5f, 0.5f));
    }

    // Only update the cube (modelMatrices[4])
    if (modelMatrices.size() > 4) {
        glm::mat4 identity = glm::mat4(1.0f);

        // Pillar's world center (from initVulkan: 2,0,0)
        glm::vec3 pillarCenter(2.0f, 0.0f, 0.0f);

        // Orbit radius (distance from pillar center)
        glm::vec3 orbitOffset(2.0f, 0.0f, 0.0f);

        // Build the model matrix: translate to pillar, rotate, translate out, scale
        modelMatrices[4] =
            glm::translate(identity, pillarCenter) *
            glm::rotate(identity, -time, glm::vec3(0.0f, 1.0f, 0.0f)) *
            glm::translate(identity, orbitOffset) *
            glm::scale(identity, glm::vec3(0.5f, 0.5f, 0.5f));
    }

    memcpy(uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
}
```

At the end as it appears in the Img. 2.10 we got to render the two pillars and cubes rotating
to their corresponding pillars.

![Render two pillars and cubes rotating](<../markdown-resources/Complete Real Time Graphics Lab Book (1 - 8)/img3-2.png> "Render two pillars and cubes rotating")

- Test data
N/A

- Sample output
A render of two pillars with cubes rotating alongside their respective pillars y-axis.

- Reflection

    - *Reflect on what you have learnt from this exercise.*
      I learn how to make an object rotate, while assigning its center of rotation as an offset (the pillar’s position or center).

    - *Did you make any mistakes?*
      Yes, I had problems rendering the position; I discovered it was due to the offset I implemented on the CreateCube functions. It distorted the position a lot.

    - *In what way has your knowledge improved?*
      That the use of push constants helps with the rendering of multiple objects as shown in Lab1, and how to change direction of rotations.

- Questions

    - *Is there anything you would like to ask?*
      No.

### Exercise 3: Advanced Rotation - Tangent to Path

#### Question
1. Goal: Transform the small rotating cube into a long, slender stick by applying an appropriate scaling transformation. Then, rotate the stick around an axis such that it remains tangent to its circular rotation path at all times, as illustrated below. Once
this behavior is achieved, extend your implementation to explore how the stick might move tangentially along a more general (non-circular) curve.
2. Conceptual Overview: To achieve this, the object's rotation must be calculated dynamically based on its position on the orbital path. The direction of travel (the tangent) becomes the object's "forward" vector.
3. Implementation:
    1. First, scale the orbiting cube into a long, thin stick (e.g., scale by `glm::vec3(0.2f, 1.0f, 0.2f)`).
    2. Construct required transformations and combine them together to achieve and specified task.
4. Expected Outcome: A visual effect shown below

#### Solution
For this exercise, to achieve the "tangent to path" effect for the horizontal pillar (cube) as it orbits the vertical pillar, we did the following steps:
1. Compute the position of the horizontal pillar on the orbit (centered at the vertical pillar).
2. Compute the tangent direction at that position (the direction of travel).
3. Build a rotation matrix so the horizontal pillar’s local x-axis (or z-axis) points along the tangent.
4. Apply scaling, then rotation, then translation to the orbit position.


This means that the horizontal pillar's center orbits the vertical pillar. The stick's "forward"
(local x) always points tangent to the path. The stick is scaled to be long and slender. The
horizontal pillar always remains tangent to the circular path.

```cpp
void HelloTriangleApplication::updateUniformBuffer(uint32_t currentImage) {
    static auto startTime = std::chrono::high_resolution_clock::now();
    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float>(currentTime - startTime).count();

    UniformBufferObject ubo{};
    ubo.view = glm::lookAt(glm::vec3(3.0f, 8.0f, 8.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    /*ubo.view = glm::lookAt(
        glm::vec3(0.0f, 15.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, -1.0f)
    );*/
    ubo.proj = glm::perspective(glm::radians(45.0f), swapChainExtent.width / (float)swapChainExtent.height, 0.1f, 50.0f);
    ubo.proj[1][1] *= -1;

    if (modelMatrices.size() > 2) {
        glm::mat4 identity = glm::mat4(1.0f);

        // --- General curve definition (example: ellipse, lemniscate, Bezier, etc.) ---
        // Example: Ellipse in XZ plane
        float t = fmod(time * 0.25f, 1.0f); // parameter in [0,1], adjust speed as needed
        float a = 3.0f; // semi-major axis
        float b = 1.5f; // semi-minor axis

        // Parametric position on the ellipse
        float theta = t * glm::two_pi<float>();
        glm::vec3 curvePos = glm::vec3(a * cos(theta), 0.0f, b * sin(theta));

        // Tangent (derivative of position w.r.t. theta)
        glm::vec3 tangent = glm::normalize(glm::vec3(
            -a * sin(theta),
            0.0f,
            b * cos(theta)
        ));

        // Up vector (Y axis)
        glm::vec3 up(0.0f, 1.0f, 0.0f);

        // Right vector (perpendicular to tangent and up)
        glm::vec3 right = glm::normalize(glm::cross(up, tangent));

        // Build rotation matrix: tangent = local X, up = local Y, right = local Z
        glm::mat4 rotation = glm::mat4(1.0f);
        rotation[0] = glm::vec4(tangent, 0.0f); // local X (forward)
        rotation[1] = glm::vec4(up, 0.0f);      // local Y
        rotation[2] = glm::vec4(right, 0.0f);   // local Z

        // Compose model matrix: translate to curvePos, rotate, scale
        modelMatrices[2] =
            glm::translate(identity, curvePos) *
            rotation *
            glm::scale(identity, glm::vec3(3.0f, 0.5f, 0.5f));
    }

    memcpy(uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
}
```

Now, the exercise asked us to make the orbit a non-circular one. So what I did was as an
example to make an ellipsis. We needed a theta variable and modified the semi-major and
semi-minor axis in order to get a curve position and the tangent. The curvePosition variable
will be used on the translation vec3, and the tangent as part of the rotation matrix. Then we
built the matrix that will be used to rotate in the ellipsis.


If I divide step by step the process, it was:
To move the horizontal pillar tangentially along a general (non-circular) curve, you need to:
1. Define the curve as a function of a parameter (e.g., t in [0,1] or time).
2. Compute the position on the curve for the current parameter.
3. Compute the tangent (derivative) at that position.
4. Build the model matrix so the horizontal pillar’s "forward" axis aligns with the tangent, and its center is at the curve position.

![Final render of the second pillar rotating on a tangent to the vertical pillar’s y-axis](<../markdown-resources/Complete Real Time Graphics Lab Book (1 - 8)/img3-3.png> "Final render of the second pillar rotating on a tangent to the vertical pillar’s y-axis"))

- Test data
N/A

- Sample output
A final render of a horizontal pillar rotating and facing a vertical pillar in an ellipsis instead of a circle.

- Reflection

    - *Reflect on what you have learnt from this exercise.*
      I learnt that a technique for rotation is a rotation matrix, and the you must build the corresponding matrix for the final rendering: translate to the curve’s position * the new rotation matrix * the scale

    - *Did you make any mistakes?*
      Not as far as I know.

    - *In what way has your knowledge improved?*
      In that, the use of a rotation matrix can help to rotate objects.

- Questions

    - *Is there anything you would like to ask?*
      Yes. Why is there no Z-depth applied? Like when a rotation is behind the pillar it just doesn’t show, but I know it is rotating along the pillar’s y-axis if I put the camera from a top down view.

     ![Depth problem](<../markdown-resources/Complete Real Time Graphics Lab Book (1 - 8)/img3-3-depth.png> "Depth problem")]

### Exercise 4: ANIMATING A SOLAR SYSTEM

#### Question
1. Goal: Create a simple solar system with a Sun, Earth, and Moon. This is a classic exercise in hierarchical transformation
2. Implementation:
    1. You will need three objects, all potentially using a cube mesh you created in Lab 1 or using the sphere mesh you load using the AssImp library in Lab 2.
    2. Sun: Place a large, scaled cube/sphere at the origin. It can have a slow axial rotation.
    3. Earth: The Earth's transformation is relative to the Sun.
        1. Start with the Sun's model matrix.
        2. Apply an orbital rotation (for Earth's orbit around the Sun).
        3. Apply a translation to define Earth's orbital distance.
        4. Apply an axial rotation (for Earth's day/night cycle).
        5. Apply a scale for Earth's size.
        4. Moon: The Moon's transformation is relative to the Earth.
    1. Start with the final transformation you applied to the Earth.
    2. Apply the Moon's orbital rotation and translation relative to the Earth.
    3. Apply the Moon's scale.
3. Expected Outcome: A scene showing a small "moon" orbiting a medium "earth," which in turn orbits a large central "sun."

#### Solution
For this exercise, I had to change the creation of meshes and matrices on initVulkan andcreate three spheres. One will be the sun rotating along its y- axis will be the earth, and rotating alongside the earth will be the moon.

First, I only focused on the Sun and the Earth. The Earth’s transform would be hierarchical.It starts with Sun’s model, then orbits, then translates, then rotates on its own axis, then scales.

```cpp
std::vector<GeometryUtils::MeshData> meshes = {
    GeometryUtils::CreateSphere(1.0f, 32, 16, glm::vec3(0.0f)), // Sun
    GeometryUtils::CreateSphere(1.0f, 32, 16, glm::vec3(0.0f)), // Earth
    GeometryUtils::CreateSphere(1.0f, 32, 16, glm::vec3(0.0f))  // Moon (will be handled in next prompt)
};

// Clear global vectors
meshBuffers.clear();
modelMatrices.clear();

// Initial transforms (will be updated every frame)
modelMatrices.push_back(glm::mat4(1.0f)); // Sun
modelMatrices.push_back(glm::mat4(1.0f)); // Earth
modelMatrices.push_back(glm::mat4(1.0f)); // Moon
```

On the updateUnifromBuffer is where most of these changes were made, after I finished the Earths rotation and got a render, then I just had to add the moon’s following these steps:
1. The Moon’s model matrix starts with the Earth’s final model matrix.
2. The Moon orbits the Earth (rotation and translation).
3. The Moon is scaled down to a realistic size (0.27x Earth’s size).
4. All transformations are hierarchical, so the Moon’s movement is always relative to the Earth, which is itself moving relative to the Sun.

```cpp
void HelloTriangleApplication::updateUniformBuffer(uint32_t currentImage) {
    static auto startTime = std::chrono::high_resolution_clock::now();
    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float>(currentTime - startTime).count();

    UniformBufferObject ubo{};
    ubo.view = glm::lookAt(glm::vec3(0.0f, 20.0f, 30.0f), glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    ubo.proj = glm::perspective(glm::radians(45.0f), swapChainExtent.width / (float)swapChainExtent.height, 0.1f, 100.0f);
    ubo.proj[1][1] *= -1;

    // Sun: at origin, slow axial rotation, large scale
    glm::mat4 sunModel = glm::rotate(glm::mat4(1.0f), time * 0.2f, glm::vec3(0, 1, 0));
    sunModel = glm::scale(sunModel, glm::vec3(3.0f)); // Large sun
    modelMatrices[0] = sunModel;

    // Earth: orbit around sun, then axial rotation, then scale
    float earthOrbitRadius = 8.0f;
    float earthOrbitSpeed = time; // radians per second
    float earthAxialSpeed = time * 2.0f; // faster than orbit

    glm::mat4 earthModel = sunModel; // Start with Sun's model matrix (hierarchical)
    earthModel = glm::rotate(earthModel, earthOrbitSpeed, glm::vec3(0, 1, 0)); // Orbit around sun
    earthModel = glm::translate(earthModel, glm::vec3(earthOrbitRadius, 0, 0)); // Move out from sun
    earthModel = glm::rotate(earthModel, earthAxialSpeed, glm::vec3(0, 1, 0)); // Earth's own rotation
    earthModel = glm::scale(earthModel, glm::vec3(1.0f)); // Earth size
    modelMatrices[1] = earthModel;

    // Moon: orbit around earth, then scale
    float moonOrbitRadius = 2.0f;
    float moonOrbitSpeed = time * 4.0f; // much faster than earth's orbit
    glm::mat4 moonModel = earthModel; // Start with Earth's model matrix (hierarchical)
    moonModel = glm::rotate(moonModel, moonOrbitSpeed, glm::vec3(0, 1, 0)); // Orbit around earth
    moonModel = glm::translate(moonModel, glm::vec3(moonOrbitRadius, 0, 0)); // Move out from earth
    moonModel = glm::scale(moonModel, glm::vec3(0.27f)); // Moon size (relative to earth)
    modelMatrices[2] = moonModel;

    memcpy(uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
}
```

![Final render of the solar system with sun, earth and moon](<../markdown-resources/Complete Real Time Graphics Lab Book (1 - 8)/img3-4.png> "Final render of the solar system with sun, earth and moon")

- Test data
N/A

- Sample output
A final render of the solar system.

- Reflection

    - *Reflect on what you have learnt from this exercise.*
      I learnt that by assigning the parent object mat4 as the start matrix of the child then it becomes hierarchical and the changes made to that model e.g. rotation, translation and scale; Would be made in local space where the center is the parent.

    - *Did you make any mistakes?*
      No.

    - *In what way has your knowledge improved?*
      In learning and practice techniques for changing the local position by assigning the matrix as the start of the matrix of a child object.

- Questions

    - *Is there anything you would like to ask?*
      No.

### Exercise 5: UNDERSTANDING VIEW AND PROJECTION

#### Question
1. Goal:
    1. Construct view matrix directly inside the vertex shader using eye position, look at point, and up direction.2
    2. Construct the perspective matrix directly inside the vertex shader using the following parameters:
        1. Field of View: fovy
        2. Aspect Ratio: aspect
        3. Near clipping plane and far clipping plane: zNear, zFar
2. Implementation: Instead of constructing the view and world matrices using the GLM library in your C++ application code, you pass only the necessary parameters to the vertex shader, where the matrices are then built directly within the shader.
3. Expected Outcome: A clear, intuitive understanding of how the view and projection matrices control the camera and perspective in a 3D scene.

#### Solution
To complete this exercise, we start by changing the UniformBufferObject struct to hold the
camera parameters as it shows in the snippet below:
```cpp
struct UniformBufferObject {
    alignas(16) glm::vec3 eye;
    alignas(16) glm::vec3 lookAt;
    alignas(16) glm::vec3 up;
    alignas(4) float fovy;
    alignas(4) float aspect;
    alignas(4) float zNear;
    alignas(4) float zFar;
};
```

Then I updated updateUniformBuffer to fill these new fields.
```cpp
void HelloTriangleApplication::updateUniformBuffer(uint32_t currentImage) {
    static auto startTime = std::chrono::high_resolution_clock::now();
    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float>(currentTime - startTime).count();

    UniformBufferObject ubo{};
    ubo.eye = glm::vec3(0.0f, 20.0f, 30.0f);
    ubo.lookAt = glm::vec3(0.0f, 0.0f, 0.0f);
    ubo.up = glm::vec3(0.0f, 1.0f, 0.0f);
    ubo.fovy = glm::radians(45.0f);
    ubo.aspect = swapChainExtent.width / (float)swapChainExtent.height;
    ubo.zNear = 0.1f;
    ubo.zFar = 100.0f;

    memcpy(uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));

    // Sun: at origin, slow axial rotation, large scale
```

```glsl
#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;

layout(location = 0) out vec3 fragColor;

layout(push_constant) uniform ModelPushConstant {
    mat4 model;
} pc;

layout(set = 0, binding = 0) uniform CameraParams {
    vec3 eye;
    vec3 lookAt;
    vec3 up;
    float fovy;
    float aspect;
    float zNear;
    float zFar;
} ubo;

// Helper function to build a lookAt matrix
mat4 lookAt(vec3 eye, vec3 center, vec3 up) {
    vec3 f = normalize(center - eye);
    vec3 s = normalize(cross(f, up));
    vec3 u = cross(s, f);

    mat4 result = mat4(1.0);
    result[0][0] = s.x;
    result[1][0] = s.y;
    result[2][0] = s.z;
    result[0][1] = u.x;
    result[1][1] = u.y;
    result[2][1] = u.z;
    result[0][2] = -f.x;
    result[1][2] = -f.y;
    result[2][2] = -f.z;
    result[3][0] = -dot(s, eye);
    result[3][1] = -dot(u, eye);
    result[3][2] = dot(f, eye);
    return result;
}

// Helper function to build a perspective matrix
mat4 perspective(float fovy, float aspect, float zNear, float zFar) {
    float tanHalfFovy = tan(fovy / 2.0);
    mat4 result = mat4(0.0);
    result[0][0] = 1.0 / (aspect * tanHalfFovy);
    result[1][1] = 1.0 / (tanHalfFovy);
    result[2][2] = -(zFar + zNear) / (zFar - zNear);
    result[2][3] = -1.0;
    result[3][2] = -(2.0 * zFar * zNear) / (zFar - zNear);
    return result;
}

void main() {
    mat4 view = lookAt(ubo.eye, ubo.lookAt, ubo.up);
    mat4 proj = perspective(ubo.fovy, ubo.aspect, ubo.zNear, ubo.zFar);

    // Vulkan NDC Y is inverted
    proj[1][1] *= -1;

    gl_Position = proj * view * pc.model * vec4(inPosition, 1.0);
    fragColor = inColor;
}
```

The changes can be summarized in:
- Main C++: Only send camera parameters, not matrices, in the uniform buffer.
- Vertex Shader (GLSL): Build the view and projection matrices from those parameters inside the vertex shader. This approach matches the exercise requirements and
  gives me full control over camera math in the shader. This will be used when we control the camera position by inputs later.

![Final rendering with different view matrix and projection](<../markdown-resources/Complete Real Time Graphics Lab Book (1 - 8)/img3-5.png> "Final rendering with different view matrix and projection)

I can now control the camera in the math shader, as I explained later, this would be used in the following section. Now we have a field of view and there’s a clipping that will be shown if you move the camera position.

- Test data
N/A

- Sample output
Render of the solar system with control of the projection.

- Reflection

    - *Reflect on what you have learnt from this exercise.*
      I learnt that view matrix and projection are used in the visualization and conversion of Dimensions. Projection allows the transformation of 3D objects into 2D images, making it possible to display complex shapes on screens or paper. Also, as there are multiple types of projection depending on the requirements, this now can be modified.

    - *Did you make any mistakes?*
      No.

    - *In what way has your knowledge improved?*
      In that we can get greater control over the camera and the view and projection by assigning these values.

- Questions

    - *Is there anything you would like to ask?*
      No.

### FURTHER EXPLORATION
Implement a simple keyboard-controlled camera. Create variables for the camera's position and target. Update these variables based on keyboard input each frame and then
regenerate the view matrix using `glm::lookAt`. This is the first step toward creating interactive 3D applications.

#### Solution
For this solution, I had to create a new function that will be in the main loop but before the drawing of the frame. I followed these steps:
1. Added Camera State Variables. Add these as private members in the HelloTriangleApplication class.
2. Handle Keyboard Input by adding a new method to process keyboard input and update the camera position/target:
3. Call Input Processing Each Frame In the mainLoop() function, call processInput() before drawFrame():
4. Use Camera State in Uniform Update Update updateUniformBuffer to use the camera variables.

1. This enables interactive camera movement in your Vulkan application

```cpp
// --- Camer State ---
glm::vec3 cameraPos = glm::vec3(0.0f, 20.0f, 30.0f);
glm::vec3 cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
```

```cpp
// --- Input Functions ---
void processInput(GLWwindow* window);
```

```cpp
void HelloTriangleApplication::mainLoop() {
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        processInput(window); // Process input before drawing
        drawFrame();
    }
    vkDeviceWaitIdle(device);
}
```

The result was not convincing due to the speed of the camera movement. I also added changes to dynamically adjust the camera speed by using shift in the keyboard and the ctrl key.

Below, in the code snippet is the improved processInput function. This version uses Left Shift to double the speed and Left Control to halve it and the Left Ctrl to reduce by double the speed. The default speed has also been reduced for finer control.

It was thanks to glfGetKey function that easily I could read the user’s input and then change the corresponding values of the camera position and tangent.

```cpp
// --- Input Functions ---
void HelloTriangleApplication::processInput(GLFWwindow* window) {
    float baseSpeed = 0.1f; // Reduced default speed for finer control
    float cameraSpeed = baseSpeed;

    // Speed up with Shift, slow down with Ctrl
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
        cameraSpeed *= 2.0f;
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) {
        cameraSpeed *= 0.5f;
    }

    // Forward/Backward (W/S)
    glm::vec3 forward = glm::normalize(cameraTarget - cameraPos);
    glm::vec3 right = glm::normalize(glm::cross(forward, cameraUp));

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        cameraPos += cameraSpeed * forward;
        cameraTarget += cameraSpeed * forward;
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        cameraPos -= cameraSpeed * forward;
        cameraTarget -= cameraSpeed * forward;
    }

    // Left/Right (A/D)
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        cameraPos -= cameraSpeed * right;
        cameraTarget -= cameraSpeed * right;
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        cameraPos += cameraSpeed * right;
        cameraTarget += cameraSpeed * right;
    }

    // Up/Down (Q/E)
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
        cameraPos += cameraSpeed * cameraUp;
        cameraTarget += cameraSpeed * cameraUp;
    }
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
        cameraPos -= cameraSpeed * cameraUp;
        cameraTarget -= cameraSpeed * cameraUp;
    }
}
```

![Final render with camera control](<../markdown-resources/Complete Real Time Graphics Lab Book (1 - 8)/img3-6.png> "Final render with camera control")

- Test data
N/A

- Sample output
A cylinder rendered with the createCylinder function.

- Reflection

    - *Reflect on what you have learnt from this exercise.*
      I learnt that by creating a function specified to controlling the camera and reading inputs in the main loop then I can control it.

    - *Did you make any mistakes?*
      Not as far as I know.

    - *In what way has your knowledge improved?*
      In that now I know how to apply inputs and control the translation of the camera, if I follow this logic then applying rotation with the mouse’s input would also be possible

- Questions

    - *Is there anything you would like to ask?*
      No.

### Final Reflection
Now I have further understood the matrices use cases and transformations. I now know
that to rotate an object I can use the rotate function on glf and use more mathematics-
based answers like multiplying a rotation matrix to create a new model matrix that rotates
by each frame drawn. I also learnt about the parent-child transformations depending on
assigning instead of the identity matrix, the parent, and how to travel along an ellipsis.

Now thanks to also being able to move the camera around, I can see the application of the
clipping space and the projections.

---

## Lab book Chapter 4: Lighting

So far, the objects in our scenes have been rendered with flat, unshaded colours. This lab introduces the next critical step in creating realistic 3D graphics: lighting. You will learn how
to simulate the way light interacts with object surfaces to create highlights and a sense of volume. We will implement the foundational Gouraud Shading and Phong Shading models,
which are computationally efficient and widely used techniques for calculating lighting. This will involve modifying vertex data, expanding our uniform buffers, and writing more complex GLSL shaders.

### Exercise 1: PREPARING FOR LIGHTING

#### Question
1. Goal: Update the application to support vertex normals and a new UBO for lighting data.
2. Implementation:
    1. Modify Vertex Struct: Add a normal vector to your C++ Vertex struct.
       ```cpp
       struct Vertex {
          glm::vec3 pos;
          glm::vec3 color;
          glm::vec3 normal; // New
       };
       ```
    1. Update Vertex Input Description: Add a new `VkVertexInputAttributeDescription` for the normal vector. Remember to update the location and offset accordingly.
    2. Define a Cube Data: Update the cube model data you created in Lab 1 with normal vectors added to your cube's vertices. Since a vertex shared between two faces (e.g., a corner) must have a different normal for each face, you can no longer use an index buffer to share vertices. You may need to define a full
       list of 36 vertices (6 faces * 2 triangles/face * 3 vertices/triangle) and provide the correct normal for each vertex (e.g., for the top face, all 6 vertices will have a normal of {0.0f, 1.0f, 0.0f}).
        1. **Update UBO**: Modify your Uniform Buffer Object struct in C++.
           ```cpp
           struct UniformBufferObject {
              alignas(16) glm::mat4 model;
              alignas(16) glm::mat4 view;
              alignas(16) glm::mat4 proj;
              alignas(16) glm::vec3 lightPos;
              alignas(16) glm::vec3 eyePos;
           };
           ```
        2. For per-Vertex lighting, update the uniform UniformBufferObject in the vertex shader to include new data information needed for lighting, such as lightPos and eye position.
           ```glsl
           layout(binding = 0) uniform UniformBufferObject {
              ... ...
              vec3 lightPos;
              vec3 eyePos;
           } ubo;
           ```

#### Solution
For this solution, first I had to create a edit the UniformBufferObject in order to to add the
eyePosition and the lightPosition.

```cpp
struct UniformBufferObject {
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
    alignas(16) glm::vec3 lightPos;
    alignas(16) glm::vec3 eyePos;
};

struct ModelPushConstant {
    glm::mat4 model;
};
```

After that I of course had to also change the shader.vert in order to keep everything working as seen on the code snippet below.

```glsl
#version 450

layout(push_constant) uniform PushConstant {
    mat4 model;
} pc;

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
    vec3 lightPos;
    vec3 eyePos;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;

layout(location = 0) out vec3 fragColor;

void main() {
    gl_Position = ubo.proj * ubo.view * pc.model * vec4(inPosition, 1.0);
    fragColor = inColor;
}
```

Because of the exercise’s instructions I also had to change the createCube function on
GeometryUtils so now the cube can have normal vectors in their vertices. Since a vertex
shared between two faces must have a different normal for each face, I needed to define a
full list of 36 vertices (6 faces * 2 triangles/face * 3 vertices/triangle) and provide the correct
normal for each vertex.

```cpp
GeometryUtils::MeshData GeometryUtils::CreateCube(float size, const glm::vec3& offset) {
    MeshData mesh;
    float h = size * 0.5f;

    // // Face normals
    glm::vec3 normals[6] = {
        {-1.0f,  0.0f,  0.0f}, // -X
        { 1.0f,  0.0f,  0.0f}, // +X
        { 0.0f, -1.0f,  0.0f}, // -Y
        { 0.0f,  1.0f,  0.0f}, // +Y
        { 0.0f,  0.0f, -1.0f}, // -Z
        { 0.0f,  0.0f,  1.0f}  // +Z
    };

    // // Face colors
    glm::vec3 colors[6] = {
        {1.0f, 0.0f, 0.0f}, // -X (red)
        {0.0f, 1.0f, 0.0f}, // +X (green)
        {0.0f, 0.0f, 1.0f}, // -Y (blue)
        {1.0f, 1.0f, 0.0f}, // +Y (yellow)
        {1.0f, 0.0f, 1.0f}, // -Z (magenta)
        {0.0f, 1.0f, 1.0f}  // +Z (cyan)
    };

    // // 6 faces, 2 triangles per face, 3 vertices per triangle
    struct Face {
        glm::vec3 v[4];
    };

    Face faces[6] = {
        // -X
        { { {-h, -h, -h}, {-h, -h,  h}, {-h,  h,  h}, {-h,  h, -h} } },
        // +X
        { { { h, -h,  h}, { h, -h, -h}, { h,  h, -h}, { h,  h,  h} } },
        // -Y
        { { {-h, -h,  h}, {-h, -h, -h}, { h, -h, -h}, { h, -h,  h} } },
        // +Y
        { { {-h,  h, -h}, {-h,  h,  h}, { h,  h,  h}, { h,  h, -h} } },
        // -Z
        { { { h, -h, -h}, {-h, -h, -h}, {-h,  h, -h}, { h,  h, -h} } },
        // +Z
        { { {-h, -h,  h}, { h, -h,  h}, { h,  h,  h}, {-h,  h,  h} } }
    };

    // // For each face, add 2 triangles (6 vertices) with correct normal
    for (int i = 0; i < 6; ++i) {
        // // Triangle 1
        mesh.vertices.push_back(GeometryVertex{ faces[i].v[0] + offset, colors[i], normals[i] });
        mesh.vertices.push_back(GeometryVertex{ faces[i].v[1] + offset, colors[i], normals[i] });
        mesh.vertices.push_back(GeometryVertex{ faces[i].v[2] + offset, colors[i], normals[i] });
        // // Triangle 2
        mesh.vertices.push_back(GeometryVertex{ faces[i].v[2] + offset, colors[i], normals[i] });
        mesh.vertices.push_back(GeometryVertex{ faces[i].v[3] + offset, colors[i], normals[i] });
        mesh.vertices.push_back(GeometryVertex{ faces[i].v[0] + offset, colors[i], normals[i] });
    }

    // // No index buffer needed; use direct vertex order
    mesh.indices.resize(36);
    for (uint32_t i = 0; i < 36; ++i) mesh.indices[i] = i;

    return mesh;
}
```

Then, I also changed the updateUniformBuffer so the camera could see the cube, thelocation was still as it was on the previous workshop.

```cpp
void HelloTriangleApplication::updateUniformBuffer(uint32_t currentImage) {
    static auto startTime = std::chrono::high_resolution_clock::now();
    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float>(currentTime - startTime).count();

    UniformBufferObject ubo{};
    ubo.view = glm::lookAt(glm::vec3(5.0f, 5.0f, 5.0f), glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    ubo.proj = glm::perspective(glm::radians(45.0f), swapChainExtent.width / (float)swapChainExtent.height, 0.1f, 100.0f);
    ubo.proj[1][1] *= -1;

    memcpy(uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
}
```

After the changes I then just call for the creation of the cube and stored its matrix.

```cpp
// Replace the mesh creation in initVulkan with:
std::vector<GeometryUtils::MeshData> meshes = {
    GeometryUtils::CreateCube(glm::vec3(0.0f)) // Cube
};

// Clear global vectors
meshBuffers.clear();
modelMatrices.clear();

//Initial transforms (will be updated every frame)
modelMatrices.push_back(glm::mat4(1.0f)); // Cube
```

Now, a cube was rendered with its normals

![Final render of the cube](<../markdown-resources/Complete Real Time Graphics Lab Book (1 - 8)/img4-1.png> "Final render of the cube")

- Test data
N/A

- Sample output
A render of a cube with normals.

- Reflection

    - *Reflect on what you have learnt from this exercise.*
      I just had to follow the instructions. Because lighting depends on normal of a mesh, f course we needed to add them to the cubes as information

    - *Did you make any mistakes?*
      No.

    - *In what way has your knowledge improved?*
      In how to apply the normals to a mesh.

- Questions

    - *Is there anything you would like to ask?*
      No.

### Exercise 2: PER-VERTEX DIFFUSE LIGHTING

#### Question
1. Goal: Implement a basic ambient and diffuse lighting model directly in the vertex shader.
2. Implementation:
    1. Vertex Shader (shader.vert):
        1. Calculate the lighting and pass the final colour to the fragment shader.
        2. In main(), perform the lighting calculation:
        ```glsl
        // Transform position and normal to world space
        vec3 worldPos = (ubo.model * vec4(inPosition, 1.0)).xyz;
        vec3 worldNormal = mat3(transpose(inverse(ubo.model)))*inNormal;

        // Define light and material properties
        vec3 lightColor = vec3(1.0, 1.0, 1.0); // Light color
        vec3 ambientMaterial = vec3(0.2, 0.1, 0.2); // Ambient light component

        // Diffuse calculation
        vec3 norm = normalize(worldNormal);
        vec3 lightDir = normalize(ubo.lightPos - worldPos);
        float diff = max(dot(norm, lightDir), 0.0);
        vec3 diffuse = diff * lightColor;

        // Combine and pass to fragment shader
        vec3 diffMaterial=vec3(1.0);
        fragColor = ambientMaterial* lightColor;
        fragColor += diffMaterial* lightColor* diffuse;
        ```
    2. Fragment Shader (shader. frag):
        1. Just receives the interpolated color and outputs it.
        ```glsl
        layout(location = 0) in vec3 fragColor;
        layout(location = 0) out vec4 outColor;

        void main() {
           outColor = vec4(fragColor, 1.0);
         }
       ```
3. Expected Outcome: The cube's shading may appear faceted and unnatural, an artifact that is particularly noticeable across large triangles. This occurs because colour values are calculated at the vertices and then linearly interpolated across the polygon's surface. Experimenting with different light positions and material colours will demonstrate how these parameters influence the final visual output.

#### Solution
So, at the call per draw in updateUniformBuffer I had to add information about the light
position to pass it to the vertex shader as the value lightPos. In this case I pass it to the
vec3( 10.0f, 10.0f, 10.0f) position. I also added the eyePos value the same one as the lookAt
of the ubo.view.

```cpp
void HelloTriangleApplication::updateUniformBuffer(uint32_t currentImage) {
    static auto startTime = std::chrono::high_resolution_clock::now();
    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float>(currentTime - startTime).count();

    UniformBufferObject ubo{};
    ubo.view = glm::lookAt(glm::vec3(5.0f, 5.0f, 5.0f), glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    ubo.proj = glm::perspective(glm::radians(45.0f), swapChainExtent.width / (float)swapChainExtent.height, 0.1f, 100.0f);
    ubo.proj[1][1] *= -1;

    // Set the light position (eg., above and to the side of the cube)
    ubo.lightPos = glm::vec3(10.0f, 10.0f, 10.0f);

    // Set the eye/camera position (should match the camera used in view matrix)
    ubo.eyePos = glm::vec3(5.0f, 5.0f, 5.0f);

    memcpy(uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
}
```

Because of those changes I then had to modify the vertex shader as stated on the exercise,
adding the reading of information lightPos, and eyePos. First we transform the position and
normal to word space. Then we define the light and material properties. Calculate the
diffuse and then we combined them and pass them to the fragment shader.

```glsl
#version 450

layout(push_constant) uniform PushConstants {
    mat4 model;
} pc;

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
    vec3 lightPos;
    vec3 eyePos;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 inNormal;

layout(location = 0) out vec3 fragColor;

void main() {
    // Transform position and normal to world space
    vec3 worldPos = (pc.model * vec4(inPosition, 1.0)).xyz;
    vec3 worldNormal = mat3(transpose(inverse(pc.model))) * inNormal;

    // Define light and material properties
    vec3 lightColor = vec3(1.0, 1.0, 1.0); // Light color
    vec3 ambientMaterial = vec3(0.2, 0.1, 0.2); // Ambient light component

    // Diffuse calculation
    vec3 norm = normalize(worldNormal);
    vec3 lightDir = normalize(ubo.lightPos - worldPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;

    // Combine and pass to fragment shader
    vec3 diffMaterial = vec3(1.0);
    fragColor = ambientMaterial * lightColor;
    fragColor += diffMaterial * lightColor * diffuse;

    gl_Position = ubo.proj * ubo.view * pc.model * vec4(inPosition, 1.0);
}
```

At the end we had a final rendering the previous cube but now with light applied via the
vertex shader

![Final render of the cube with per-vertex lighting](<../markdown-resources/Complete Real Time Graphics Lab Book (1 - 8)/img4-2.png> "Final render of the cube with per-vertex lighting")

- Test data
N/A

- Sample output
A render of a cube with light.

- Reflection

    - *Reflect on what you have learnt from this exercise.*
      I learnt that you can and must pass information on to the uniform buffered object to the vertex shader.

    - *Did you make any mistakes?*
      No.

    - *In what way has your knowledge improved?*
      It improved in the practice of applying algorithms on the vertex shader. And then pass information to the fragment shader to get the final colors.

- Questions

    - *Is there anything you would like to ask?*
      No.

### Exercise 3: PER-FRAGMENT DIFFUSE LIGHTING

#### Question
1. Goal: Improve visual quality by moving the lighting logic to the fragment shader, allowing for smoother, more accurate calculations.
2. Implementation:
    1. Vertex Shader (shader.vert):
        1. The vertex shader's job is now to pass the necessary data (world position, world normal, and vertex colour) to the fragment shader.
        2. Define ‘out’ variables for these values.
        ```glsl
        layout(location = 0) out vec3 fragColor;
        layout(location = 1) out vec3 fragWorldPos;
        layout(location = 2) out vec3 fragWorldNormal;
        ```
        3. In main(), calculate and pass the data to fragment shader:
        ```glsl
        fragWorldPos = (ubo.model * vec4(inPosition, 1.0)).xyz;
        fragWorldNormal = mat3(transpose(inverse(ubo.model))) * inNormal;
        fragColor = inColor;
        ```
    2. Fragment Shader (shader.frag):
        1. Add the uniform UniformBufferObject block to the top of your fragment shader (shader.frag)
        2. Define corresponding ‘in’ variables to receive the interpolated data.
        3. In main(), perform the same ambient and diffuse calculation done inthe vertex shader in
        4. Exercise 2, but using the interpolated ‘in’ variables to calculate the output colour as the addition of ambient and diffuse colour.
3. Expected Outcome: The lighting on the cube should now appear much smoother and more realistic, as the calculation is done for every pixel. Experimenting with different light positions and material colours will demonstrate how theseparameters influence the final visual output.

#### Solution
So, for this exercise, I had to change the core of the problem solving to the fragment shader instead of the vertex shader

```glsl
#version 450

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
    vec3 lightPos;
    vec3 eyePos;
} ubo;

// Receive interpolated data from vertex shader
layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragWorldPos;
layout(location = 2) in vec3 fragWorldNormal;

layout(location = 0) out vec4 outColor;

void main() {
    // Define light and material properties
    vec3 lightColor = vec3(1.0, 1.0, 1.0); // Light color
    vec3 ambientMaterial = vec3(0.2, 0.1, 0.2); // Ambient light component

    // Diffuse calculation
    vec3 norm = normalize(fragWorldNormal);
    vec3 lightDir = normalize(ubo.lightPos - fragWorldPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;

    // Combine ambient and diffuse
    vec3 diffMaterial = vec3(1.0);
    vec3 color = ambientMaterial * lightColor;
    color += diffMaterial * lightColor * diffuse;

    outColor = vec4(color, 1.0);
}
```

After applying the changes to the fragment shader so it can have both ambient and diffuse
lighting. The vertex shader becomes kind of like a bridge to cross and pass information from
the CPU to the vertex and then de fragment shader.

```glsl
#version 450

layout(push_constant) uniform PushConstants {
    mat4 model;
} pc;

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
    vec3 lightPos;
    vec3 eyePos;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 inNormal;

// Pass necessary data to fragment shader
layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 fragWorldPos;
layout(location = 2) out vec3 fragWorldNormal;

void main() {
    // Calculate world position and normal using pc.model
    fragWorldPos = (pc.model * vec4(inPosition, 1.0)).xyz;
    fragWorldNormal = mat3(transpose(inverse(pc.model))) * inNormal;
    fragColor = inColor;

    gl_Position = ubo.proj * ubo.view * pc.model * vec4(inPosition, 1.0);
}
```

As the expected outcome now the calculation of the light and the combination of both,
ambient and diffuse and much more rich lighting appears.

![Final render of the cube with per-fragment lighting](<../markdown-resources/Complete Real Time Graphics Lab Book (1 - 8)/img4-3.png> "Final render of the cube with per-fragment lighting")

- Test data
N/A

- Sample output
A render of a cube with ambient light + diffuse lighting via the fragment shader.

- Reflection

    - *Reflect on what you have learnt from this exercise.*
      I learnt to change the approach and then use the fragment shader as the main culprit of coloring and rendering light.

    - *Did you make any mistakes?*
      No.

    - *In what way has your knowledge improved?*
      It improved in the sense that now I can use the fragment shader to apply the mathematics formulas to render lightinge.

- Questions

    - *Is there anything you would like to ask?*
      No.

### Final Reflection
This one was difficult but entreating I learnt a lot about lighting process and more about the vulkan pipeline and how it is difficult to correctly use the push constants. The hard work paid off at the end.

### Exercise 4: ADDING PER-VERTEX SPECULAR LIGHTING

#### Question                        
1. Goal: Complete the reflection model implemented in Exercise 2 by adding specular highlights to the vertex shader.
    2. Implementation:
        1. Vertex Shader (shader.vert):
            1. Add to the code from Exercise 2. After the diffuse calculation, implement the specular component:
            ```glsl
            // Specular calculation
            vec3 lightDir = ... ...;
            vec3 viewDir = normalize(ubo.eyePos - worldPos);
            vec3 reflectDir = normalize(reflect(-lightDir, norm));
            float shininess = 32.0;
            float spec = pow(max(dot(reflectDir, viewDir), 0.0), shininess);
            vec3 specMaterial=vec3(1.0);
            vec3 specular = specMaterial *lightColor*spec;
            // Combine all components
            fragColor += specular;
            ```
2. Fragment Shader (shader.frag):
1. No changes are needed from Exercise 2. It will simply render the final interpolated colour.
3. Expected Outcome: The cube will now have highlights if relevant lighting parameters in the light equation are properly configured. Experimenting with different light positions, eye position and material colours will demonstrate how these parameters influence the final visual output.

#### Solution
So, according to the exercise now, I had to add specular lighting to the model. The calculations had to be made on the vertex shader. So I returned to how it was made on the
exercise 2, to leave the fragment shader as it was previously. Then I applied the calculation to and passed it to the fragColor to render it on the fragment shader.

```glsl
#version 450

layout(push_constant) uniform PushConstants {
    mat4 model;
} pc;

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
    vec3 lightPos;
    vec3 eyePos;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 inNormal;

layout(location = 0) out vec3 fragColor;

void main() {
    // Transform position and normal to world space
    vec3 worldPos = (pc.model * vec4(inPosition, 1.0)).xyz;
    vec3 worldNormal = mat3(transpose(inverse(pc.model))) * inNormal;

    // Define light and material properties
    vec3 lightColor = vec3(1.0, 1.0, 1.0); // Light color
    vec3 ambientMaterial = vec3(0.2, 0.1, 0.2); // Ambient light component

    // Diffuse calculation
    vec3 norm = normalize(worldNormal);
    vec3 lightDir = normalize(ubo.lightPos - worldPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;

    // Specular calculation
    vec3 viewDir = normalize(ubo.eyePos - worldPos);
    vec3 reflectDir = normalize(reflect(-lightDir, norm));
    float shininess = 32.0;
    float spec = pow(max(dot(reflectDir, viewDir), 0.0), shininess);
    vec3 specMaterial = vec3(1.0);
    vec3 specular = specMaterial * lightColor * spec;

    // Combine and pass to fragment shader
    vec3 diffMaterial = vec3(1.0);
    fragColor = ambientMaterial * lightColor;
    fragColor += diffMaterial * lightColor * diffuse;
    fragColor += specular;

    gl_Position = ubo.proj * ubo.view * pc.model * vec4(inPosition, 1.0);
}
```

In the end I got the lighting with the specular lighting.

![Final render of the cube with per-vertex specular lighting](<../markdown-resources/Complete Real Time Graphics Lab Book (1 - 8)/img4-4.png> "Final render of the cube with per-vertex specular lighting")

- Test data
N/A

- Sample output
A render of a cube with specular lighting calculated on the vertex shader.

- Reflection

    - *Reflect on what you have learnt from this exercise.*
      I learnt how to apply specular lighting in the vertex shader.

    - *Did you make any mistakes?*
      No.

    - *In what way has your knowledge improved?*
      In that now I just had to add and apply the calculations for specular lighting on the vertex shader. And use the fragment one as a painter of vec 3.

- Questions

    - *Is there anything you would like to ask?*
      No.

### Exercise 5: ADDING PER-FRAGMENT SPECULAR LIGHTING

#### Question
1. Goal: Complete the reflection model implemented in Exercise 3 by adding specular highlights to the fragment shader.  
2. Implementation:
    1. Vertex Shader (shader.vert):
        1. No change to the vertex shader used in Exercise 3 is required.
    2. Fragment Shader (shader.frag):
        1. Update the main() used in Exercise 3 by calculating the specular component and add it to the ambient and diffuse components calculated in Exercise 3 on per-fragment diffuse lighting.
3. Expected Outcome: The cube should now have a bright, shiny highlight on the faces angled correctly towards the light source and the camera. Experimenting with different light positions, eye position and material colours will demonstrate how these parameters influence the final visual output.

#### Solution
So, according to the exercise, I once again had to add specular lighting to the model. The calculations had to be made on the fragment shader. So I returned to how it was made on the exercise 3, to leave the vertex shader as it was previously and just update the fragment one. Then I applied the calculation to and passed it to the fragColor to render it on the fragment shader.

```glsl
#version 450

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
    vec3 lightPos;
    vec3 eyePos;
} ubo;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragWorldPos;
layout(location = 2) in vec3 fragWorldNormal;

layout(location = 0) out vec4 outColor;

void main() {
    // Ambient component
    vec3 lightColor = vec3(1.0, 1.0, 1.0);
    vec3 ambientMaterial = vec3(0.2, 0.1, 0.2);
    vec3 ambient = ambientMaterial * lightColor;

    // Diffuse component
    vec3 norm = normalize(fragWorldNormal);
    vec3 lightDir = normalize(ubo.lightPos - fragWorldPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffMaterial = fragColor;
    vec3 diffuse = diffMaterial * lightColor * diff;

    // Specular component
    vec3 viewDir = normalize(ubo.eyePos - fragWorldPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float shininess = 32.0;
    float spec = pow(max(dot(reflectDir, viewDir), 0.0), shininess);
    vec3 specMaterial = vec3(1.0);
    vec3 specular = specMaterial * lightColor * spec;

    // Combine all components
    vec3 result = ambient + diffuse + specular;
    outColor = vec4(result, 1.0);
}
```

After applying the changes I got the lighting with the specular lighting.

![Final render of the cube with per-fragment specular lighting](<../markdown-resources/Complete Real Time Graphics Lab Book (1 - 8)/img4-5.png> "Final render of the cube with per-fragment specular lighting")

- Test data
N/A

- Sample output
A render of a cube with specular lighting calculated on the fragment shader.

- Reflection

    - *Reflect on what you have learnt from this exercise.*
      I learnt how to apply specular lighting in the fragment shader.

    - *Did you make any mistakes?*
      No.

    - *In what way has your knowledge improved?*
      In that now I just had to add and apply the calculations for specular lighting on the fragment shader. And use the vertex one as a bridge to pass the information.

- Questions

    - *Is there anything you would like to ask?*
      No.

### Exercise 6: MULTIPLE LIGHTS AND MATERIALS

#### Question
Draw three cube objects with three different surface material properties. Illuminate these cubes with two light sources: one is a static white light; the other is a red light rotating
dynamically by the y-axis.

#### Solution
First I created another push constant for the materials

```cpp
struct UniformBufferObject {
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 view;
    alignas(16) glm::vec3 proj;
    alignas(16) glm::vec3 lightPos1;
    alignas(16) glm::vec3 lightColor1;
    alignas(16) glm::vec3 lightPos2;
    alignas(16) glm::vec3 lightColor2;
    alignas(16) glm::vec3 eyePos;
};

struct MaterialPushConstant {
    alignas(16) glm::vec3 ambient;
    alignas(16) glm::vec3 diffuse;
    alignas(16) glm::vec3 specular;
    float shininess;
};
```

Then I had to update the updateUniformBuffer function in order to have the lighting settings and the rotation of the red light.

```cpp
void HelloTriangleApplication::updateUniformBuffer(uint32_t currentImage) {
    static auto startTime = std::chrono::high_resolution_clock::now();
    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float>(currentTime - startTime).count();

    UniformBufferObject ubo{};
    // Rotate model 90 degrees per second around the Y axis
    ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    
    // Camera setup
    ubo.view = glm::lookAt(glm::vec3(5.0f, 5.0f, 5.0f), glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    ubo.proj = glm::perspective(glm::radians(45.0f), swapChainExtent.width / (float)swapChainExtent.height, 0.1f, 100.0f);
    ubo.proj[1][1] *= -1; // Invert Y for Vulkan

    // Static white light
    ubo.lightPos1 = glm::vec3(10.0f, 10.0f, 10.0f);
    ubo.lightColor1 = glm::vec3(1.0f, 1.0f, 1.0f);

    // Rotating red light logic
    float radius = 12.0f;
    float x = radius * cos(time);
    float z = radius * sin(time);
    ubo.lightPos2 = glm::vec3(x, 8.0f, z);
    ubo.lightColor2 = glm::vec3(1.0f, 0.0f, 0.0f);

    // Specular eye position
    ubo.eyePos = glm::vec3(5.0f, 5.0f, 5.0f);

    memcpy(uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
}
```

Then, and this is the important part I just added the model matrix back to the UniformBufferObject and let the push constants be all for the material and just be one type of struct about it.

```cpp
VkPushConstantRange materialPushConstantRange{};
materialPushConstantRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
materialPushConstantRange.offset = 0;
materialPushConstantRange.size = sizeof(MaterialPushConstant);

VkPiepelineLayoutCreateInfo pipelineLayoutInfo{};
pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
pipelineLayoutInfo.setLayoutCount = 1;
pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
pipelineLayoutInfo.pushConstantRangeCount = 1;
pipelineLayoutInfo.pPushConstantRanges = &materialPushConstantRange;

```

Changed the push constants so I only read one of them instead of an array of two, that’s also why I changed it back the model from another push constant to the UniformBufferObject.

```cpp
for (size_t i = 0; i < meshBuffers.size(); ++i) {
    VkBuffer vertexBuffers[] = { meshBuffers[i].vertexBuffer };
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(commandBuffer, meshBuffers[i].indexBuffer, 0, VK_INDEX_TYPE_UINT32);

    MaterialPushConstant material{};
    if (i == 0) {
        material.ambient = glm::vec3(0.2f, 1.0f, 0.0f);
        material.diffuse = glm::vec3(0.0f, 0.1f, 0.1f);
        material.specular = glm::vec3(1.0f, 0.6f, 0.6f);
        material.shininess = 16.0f;
    } else if (i == 1) {
        material.ambient = glm::vec3(0.0f, 0.2f, 0.0f);
        material.diffuse = glm::vec3(0.1f, 0.8f, 0.1f);
        material.specular = glm::vec3(0.6f, 1.0f, 0.6f);
        material.shininess = 32.0f;
    } else {
        material.ambient = glm::vec3(0.0f, 0.0f, 0.2f);
        material.diffuse = glm::vec3(0.1f, 0.1f, 0.8f);
        material.specular = glm::vec3(0.6f, 0.6f, 1.0f);
        material.shininess = 64.0f;
    }

    vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(MaterialPushConstant), &material);

    vkCmdDrawIndexed(commandBuffer, meshBuffers[i].indexCount, 1, 0, 0, 0);
}
```

On the recordCommandBuffer function I added the different lights and the materials so ant the end I could have the different illuminations. I had to change the fragment shader in order to process the lights.

```glsl
#version 450

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec3 lightPos1;
    vec3 lightColor1;
    vec3 lightPos2;
    vec3 lightColor2;
    vec3 eyePos;
} ubo;

layout(push_constant) uniform MaterialPushConstant {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float shininess;
} material;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragWorldPos;
layout(location = 2) in vec3 fragWorldNormal;

layout(location = 0) out vec4 outColor;

void main() {
    vec3 norm = normalize(fragWorldNormal);
    vec3 viewDir = normalize(ubo.eyePos - fragWorldPos);

    // // Light 1 (white)
    vec3 lightDir1 = normalize(ubo.lightPos1 - fragWorldPos);
    float diff1 = max(dot(norm, lightDir1), 0.0);
    vec3 reflectDir1 = reflect(-lightDir1, norm);
    float spec1 = pow(max(dot(reflectDir1, viewDir), 0.0), material.shininess);

    vec3 ambient1 = material.ambient * ubo.lightColor1;
    vec3 diffuse1 = material.diffuse * ubo.lightColor1 * diff1;
    vec3 specular1 = material.specular * ubo.lightColor1 * spec1;

    // // Light 2 (red, rotating)
    vec3 lightDir2 = normalize(ubo.lightPos2 - fragWorldPos);
    float diff2 = max(dot(norm, lightDir2), 0.0);
    vec3 reflectDir2 = reflect(-lightDir2, norm);
    float spec2 = pow(max(dot(reflectDir2, viewDir), 0.0), material.shininess);

    vec3 ambient2 = material.ambient * ubo.lightColor2;
    vec3 diffuse2 = material.diffuse * ubo.lightColor2 * diff2;
    vec3 specular2 = material.specular * ubo.lightColor2 * spec2;

    vec3 result = ambient1 + diffuse1 + specular1 + ambient2 + diffuse2 + specular2;

    outColor = vec4(result, 1.0);
}
```

At the end I got it to finally render the light properly, and three cubes spinning around with a red light also spinning.

![Final render of the cubes with multiple lights and materials](<../markdown-resources/Complete Real Time Graphics Lab Book (1 - 8)/img4-6.png> "Final render of the cubes with multiple lights and materials")

- Test data
N/A

- Sample output
A render of three cubes with different materials and a static lighting and a red light that rotates.

- Reflection

    - *Reflect on what you have learnt from this exercise.*
      This one was difficult to implement because the push constants were not passing correctly. I learnt about that and fixed it by deleting the push constant for the model.

    - *Did you make any mistakes?*
      Yes. Push constants were not passing correctly.

    - *In what way has your knowledge improved?*
      In that when dealing with push constants it may be better to only have one due to data moving around

- Questions

    - *Is there anything you would like to ask?*
      No.

### Final Reflection
This one was difficult but entreating I learnt a lot about lighting process and more about the vulkan pipeline and how it is difficult to correctly use the push constants. The hard work paid off at the end.

---

## Lab book Chapter 5: Texture mapping

In previous labs, we've given our objects colour and simulated lighting, but they still look flat and artificial. Texture Mapping is the technique that provides the single biggest jump in visual realism. It's the process of "wrapping" a 2D image—called a texture—onto the surface of a 3D model. 

This allows us to add intricate details like wood grain, brick patterns, text, or any complex surface property without increasing the geometric complexity of the model itself. This lab will guide you through the complete Vulkan workflow for loading, managing, and sampling a texture, which is one of the most fundamental skills in modern 3D graphics.

### Exercise 1: PREPARING THE APPLICATION FOR TEXTURES

#### Question
1. **Get stb_image.h**: Download the stb_image.h file from the official GitHub repository [github.com/nothings/stb](https:www.github.com/nothings/stb) and add it to your project's include directory.
2. **Implement stb_image.h**: In one of your .cpp files, before including it, you must define
STB_IMAGE_IMPLEMENTATION:
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
3. **Get a Texture**: Find a simple texture file (e.g., container.jpg, wall.png, or a simple brick pattern) and place it in a location your application can read (like the build directory).
4. **Update Vertex Struct**: Add glm::vec2 texCoord; to your C++ Vertex struct.
```cpp
struct Vertex {
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec3 normal;
    glm::vec2 texCoord; // New
};
```
5. **Update Vertex Input Description: Add a new VkVertexInputAttributeDescription for the texture coordinates. Update the location and offset values for all attributes to be correct.
6. Update Cube Data: This is a critical step. You must provide UV coordinates for all 36 vertices of your cube. Each face should map to the full [0, 1] range.
- glm::vec2(0.0f, 1.0f): Bottom-left
- glm::vec2(1.0f, 1.0f): Bottom-right
- glm::vec2(1.0f, 0.0f): Top-right
- glm::vec2(0.0f, 0.0f): Top-left
```cpp
For example, one face (two triangles) would look like this:// Front face
{{-0.5f, -0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}}, // Bottom-left
{{ 0.5f, -0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}}, // Bottom-right
… …
```
You must do this for all 6 faces.

#### Solution
So for this exercise I had to start by adding the correct libraries for loading images textures. In this case as the instructions said, the stb_image.h and added it to the project’s folder.

```cpp
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
```

Then I added the images on the resources folder, before that I had to convert them into .jpg files.
```
resources/
    coin.dds
    rocks.dds
    Tiles.dds
    wood.dds
```
Then I had to modify the Vertex struct to add the texture coordinates.

```cpp
struct GeometryVertex {
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec3 normal;
    glm::vec2 texCoord;

    static VkVertexInputBindingDescription getBindingDescription() {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(GeometryVertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        return bindingDescription;
    }

    static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions() {
        std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};
        attributeDescriptions[0] = { 0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(GeometryVertex, pos) };
        attributeDescriptions[1] = { 1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(GeometryVertex, color) };
        attributeDescriptions[2] = { 2, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(GeometryVertex, normal) };
        return attributeDescriptions;
    }

    static VkVertexInputAttributeDescription getTexCoordAttributeDescription() {
        VkVertexInputAttributeDescription texCoordDescription{};
        texCoordDescription.binding = 0;
        texCoordDescription.location = 3;
        texCoordDescription.format = VK_FORMAT_R32G32_SFLOAT;
        texCoordDescription.offset = offsetof(GeometryVertex, texCoord);
        return texCoordDescription;
    }
};
```

Then I had to add the new vec2 for the texture coordinates, and on the Attribute Description                                                                                                                                                                                                                                                                                                                        I also had to add it to the description’s array as the fourth attribute.
I also had to prepare the cube because now I included it’s UV mapping as an array of vec2

```cpp
GeometryUtils::MeshData GeometryUtils::CreateCube(float size, const glm::vec3& offset) {
    MeshData mesh;
    float h = size * 0.5f;

    // Face normals
    glm::vec3 normals[6] = {
        {-1.0f,  0.0f,  0.0f}, // -X
        { 1.0f,  0.0f,  0.0f}, // +X
        { 0.0f, -1.0f,  0.0f}, // -Y
        { 0.0f,  1.0f,  0.0f}, // +Y
        { 0.0f,  0.0f, -1.0f}, // -Z
        { 0.0f,  0.0f,  1.0f}  // +Z
    };

    // Face colors
    glm::vec3 colors[6] = {
        {1.0f, 0.0f, 0.0f}, // -X (red)
        {0.0f, 1.0f, 0.0f}, // +X (green)
        {0.0f, 0.0f, 1.0f}, // -Y (blue)
        {1.0f, 1.0f, 0.0f}, // +Y (yellow)
        {1.0f, 0.0f, 1.0f}, // -Z (magenta)
        {0.0f, 1.0f, 1.0f}  // +Z (cyan)
    };

    // UVs: bottom-left, bottom-right, top-right, top-left
    glm::vec2 uvs[4] = {
        {0.0f, 1.0f}, // bottom-left
        {1.0f, 1.0f}, // bottom-right
        {1.0f, 0.0f}, // top-right
        {0.0f, 0.0f}  // top-left
    };

    // 6 faces, 2 triangles per face, 3 vertices per triangle
    struct Face { glm::vec3 v[4]; };
    Face faces[6] = {
        // -X
        { { {-h, -h, -h}, {-h, -h,  h}, {-h,  h,  h}, {-h,  h, -h} } },
        // +X
        { { { h, -h,  h}, { h, -h, -h}, { h,  h, -h}, { h,  h,  h} } },
        // -Y
        { { {-h, -h,  h}, {-h, -h, -h}, { h, -h, -h}, { h, -h,  h} } },
        // +Y
        { { {-h,  h, -h}, {-h,  h,  h}, { h,  h,  h}, { h,  h, -h} } },
        // -Z
        { { { h, -h, -h}, {-h, -h, -h}, {-h,  h, -h}, { h,  h, -h} } },
        // +Z
        { { {-h, -h,  h}, { h, -h,  h}, { h,  h,  h}, {-h,  h,  h} } }
    };

    // For each face, add 2 triangles (6 vertices) with correct normal and UVs
    for (int i = 0; i < 6; ++i) {
        // Triangle 1: v0, v1, v2
        mesh.vertices.push_back(GeometryVertex{ faces[i].v[0] + offset, colors[i], normals[i], uvs[0] });
        mesh.vertices.push_back(GeometryVertex{ faces[i].v[1] + offset, colors[i], normals[i], uvs[1] });
        mesh.vertices.push_back(GeometryVertex{ faces[i].v[2] + offset, colors[i], normals[i], uvs[2] });
        // Triangle 2: v2, v3, v0
        mesh.vertices.push_back(GeometryVertex{ faces[i].v[2] + offset, colors[i], normals[i], uvs[2] });
        mesh.vertices.push_back(GeometryVertex{ faces[i].v[3] + offset, colors[i], normals[i], uvs[3] });
        mesh.vertices.push_back(GeometryVertex{ faces[i].v[0] + offset, colors[i], normals[i], uvs[0] });
    }

    mesh.indices.resize(36);
    for (uint32_t i = 0; i < 36; ++i) mesh.indices[i] = i;

    return mesh;
}
```

Then I had to run the program to see if there was any problem at all.

![Final render of the cube with texture mapping](<../markdown-resources/Complete Real Time Graphics Lab Book (1 - 8)/img5-1.png> "Final render of the cube with texture mapping")]

- Test data
N/A

- Sample output
A render of a cubes with normals.

- Reflection

    - *Reflect on what you have learnt from this exercise.*
      I just had to follow the instructions. And make sure everything was still running as intended.

    - *Did you make any mistakes?*
      No.

    - *In what way has your knowledge improved?*
      In how to apply the UV to a mesh’s faces.

- Questions

    - *Is there anything you would like to ask?*
      No.

### Exercise 2: LOADING AND CREATING VULKAN IMAGE RESOURCES

#### Question
1. Declare relevant Vulkan Objects to be created.
2. Load image with stb_image.
3. Create Staging Buffer: Create a VkBuffer and VkDeviceMemory
    1. size: imageSize
    2. usage: `VK_BUFFER_USAGE_TRANSFER_SRC_BIT`
    3. properties: `VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT`
4. Copy Data to staging Buffer:
    1. Map the buffer, mempcy the pixels data into it, and unmap.
5. Free CPU pixels: Call stbi_image_free(pixels); now that the data is on the staging buffer.
6. Create VkImage: Create your final VkImage (textureImage).
    1. mageType: VK_IMAGE_TYPE_2D
    2. extent: { (uint32_t)texWidth, (uint32_t)texHeight, 1 }
    3. mipLevels: 1
    4. arrayLayers: 1
    5. format: `VK_FORMAT_R8G8B8A8_SRGB` (Use SRGB for colour textures)
    6. tiling: `VK_IMAGE_TILING_OPTIMAL`
    7. initialLayout: `VK_IMAGE_LAYOUT_UNDEFINED`
    8. usage: `VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT`
    9. sharingMode: VK_SHARING_MODE_EXCLUSIVE
    10. samples: VK_SAMPLE_COUNT_1_BIT
7. Allocate VkImage Memory: Allocate vkDeviceMemory (texttureImageMemory) for this VkImage with VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT and bind it.
8. Create VkImageView: Create VkImageView (textureImageView) for texureImage.
    1. Image: textureImage
    2. ViewType: VK_IMAGE_TYPE_2D
    3. Format: VK_FORMAT_R8G8B8A8_SRGB
    4. SubresourceRange.aspectMask: VK_IMAGE_ASPECT_COLOR_BIT
9. Execute Transfer Commands: You need to record and submit a one-time command buffer to perform the copy.
    1. transitionImageLayout(textureImage, ..., VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    2. copyBufferToImage(stagingBuffer, textureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));                
    3. transitionImageLayout(textureImage, ...,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    4. (These transition and copy functions will record barrier and copy commands into a command buffer, which you must then submit and wait on).
10. Clean Up Staging Buffer: After the transfer is complete, destroy stagingBuffer and free stagingBufferMemory.
11. Create VkSampler: Create a VkSampler (textureSampler) to tell the shader how to read the image.
    1. magFilter: `VK_FILTER_LINEAR`
    2. minFilter: `VK_FILTER_LINEAR`
    3. addressModeU/V/W: `VK_SAMPLER_ADDRESS_MODE_REPEAT`
    4. anisotropyEnable: `VK_TRUE` (or VK_FALSE if not supported)
    5. maxAnisotropy: 16.0f (or 1.0f if disabled)
    6. borderColor: `VK_BORDER_COLOR_INT_OPAQUE_BLACK`
    7. unnormalizedCoordinates: `VK_FALSE`

#### Solution
So, for this solution I only had to follow the instructions accordingly step by step as describe per image context. First it I had to create the function for reading a texture, and then I had to load the wood image from the resources/jpg/ folder. But due to errors I still needed to complete the process.

I added the four variables in the HelloTriangle class which is the main class for this project just as the declaration of the new functions that will help the createTexture function to load and map the texture onto the meshes.

```cpp
void HelloTriangleApplication::createTextureImage() {
    int texWidth, texHeight, texChannels;

    stbi_uc* pixels = stbi_load("C:\\Users\\949145\\GitHub\\GraphicsLab5\\Vulkan(1_3)_Lab_05\\resources\\jpg\\wood.jpg", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

    if (!pixels) {
        throw std::runtime_error("Failed to load texture image!");
    }

    VkDeviceSize imageSize = texWidth * texHeight * 4;

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;

    createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingBuffer,
        stagingBufferMemory
    );

    void* data;
    vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data);
    memcpy(data, pixels, static_cast<size_t>(imageSize));
    vkUnmapMemory(device, stagingBufferMemory);

    stbi_image_free(pixels);

    createImage(texWidth, texHeight, VK_FORMAT_R8G8B8A8_SRGB,
        VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureImage, textureImageMemory
    );

    transitionImageLayout(textureImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);
    copyBufferToImage(stagingBuffer, textureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
    transitionImageLayout(textureImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);

    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
}
```

```cpp
// --- Texture Mapping Members ---
VkImage textureImage;
VkDeviceMemory textureImageMemory;
VkImageView textureImageView;
VkSampler textureSampler;
```

```cpp
/* Texture Mapping Starts */

void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);

void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);

VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);

void createTextureImageView();

void transitionImageLayout(VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout, VkImageAspectFlags aspectMask);

void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

VkCommandBuffer beginSingleTimeCommands();

void endSingleTimeCommands(VkCommandBuffer commandBuffer);

void createTextureSampler();

void createTextureImage();

/* Texture Mapping Ends */
```

The it was just about following the instructions according to each function that I had to implement. I started with the create Buffer.

```cpp
void HelloTriangleApplication::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) {
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to create buffer!");
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate buffer memory!");
    }

    vkBindBufferMemory(device, buffer, bufferMemory, 0);
}
```

```cpp
void HelloTriangleApplication::createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory) {
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateImage(device, &imageInfo, nullptr, &image) != VK_SUCCESS) {
        throw std::runtime_error("failed to create image!");
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device, image, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate image memory!");
    }

    vkBindImageMemory(device, image, imageMemory, 0);
}

VkImageView HelloTriangleApplication::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags) {
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = aspectFlags;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    VkImageView imageView;
    if (vkCreateImageView(device, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
        throw std::runtime_error("failed to create image view!");
    }

    return imageView;
}

void HelloTriangleApplication::createTextureImageView() {
    textureImageView = createImageView(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);
}
```

By the end of it I also implemented the createImage, createImageView and
createTextureView functions in order to properly map the texture on the cube.

```cpp
void HelloTriangleApplication::transitionImageLayout(VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout, VkImageAspectFlags aspectMask) {
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = aspectMask;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    }
    else {
        throw std::invalid_argument("unsupported layout transition!");
    }

    vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
    endSingleTimeCommands(commandBuffer);
}
```

By the end I only had to continue with the instructions, now it was time to implement the transitionImageLayout as well as the of beginSingleTimeCommands, endSingleTimeCommands and createTextureSampler to continue with the sahders in the next exercise.

```cpp
void HelloTriangleApplication::copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();

    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;

    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;

    region.imageOffset = { 0, 0, 0 };
    region.imageExtent = { width, height, 1 };

    vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    endSingleTimeCommands(commandBuffer);
}

VkCommandBuffer HelloTriangleApplication::beginSingleTimeCommands() {
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
}

void HelloTriangleApplication::endSingleTimeCommands(VkCommandBuffer commandBuffer) {
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphicsQueue);

    vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}

void HelloTriangleApplication::createTextureSampler() {
    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(physicalDevice, &properties);

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;

    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

    if (vkCreateSampler(device, &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture sampler!");
    }
}
```

- Test data
N/A

- Sample output
N/A

- Reflection

    - *Reflect on what you have learnt from this exercise.*
      I learnt about the needed process to map an image and how in the vulkan api you have to configure how the information passes through the entire pipeline.

    - *Did you make any mistakes?*
      No.

    - *In what way has your knowledge improved?*
      It improved in the practice of applying algorithms and function for reading images and passing the information into the buffers, and of course also create those buffers to save and pass the information.

- Questions

    - *Is there anything you would like to ask?*
      No.

### Exercise 3: BINDING AND SHADER UPDATES

#### Question
1. Update Descriptor Set Layout: Add a new binding to your `VkDescriptorSetLayoutCreateInfo`.
    1. VkDescriptorSetLayoutBinding samplerLayoutBinding{};
    2. samplerLayoutBinding.binding = 1; // 0 is already used by the UBO
    3. samplerLayoutBinding.descriptorType =
    4. VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    5. samplerLayoutBinding.descriptorCount = 1;
    6. samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    Add this to your array of bindings when creating the layout.

2. Update Descriptor Pool: Make sure your VkDescriptorPool is created with a VkDescriptorPoolSize for VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLE
3. Update Descriptor Set: When you create your VkDescriptorSet, you must now write two descriptors to it.
    1. One VkDescriptorBufferInfo for the UBO at binding = 0.
    2. One VkDescriptorImageInfo for the texture at binding = 1.
4. Update Descriptor set layout.
5. Add necessary cleanup for your new texture resources in the cleanup function to prevent memory leaks.
6. Update initVulkan()
    ```cpp
    ... ...
    createTextureImage();
    createTextureImageView();
    createTextureSampler();
    ```
7. Vertex Shader (shader.vert):
    1. Add the new input: layout(location = 3) in vec2 inTexCoord; (Adjust location as needed)
    2. Add a new output: layout(location = 3) out vec2 fragTexCoord;
    3. In main(), pass it through: fragTexCoord = inTexCoord;
8. Fragment Shader (shader.frag):
    1. Add the new uniform sampler: layout(binding = 1) uniform sampler2D texSampler;
    2. Add the new input: layout(location = 3) in vec2 fragTexCoord;
    3. In main(), sample the texture and combine it with your lighting.
    ```glsl
    .... ...
    vec4 texColor = texture(texSampler, fragTexCoord);
    // ... (Your lighting calculations: ambient, diffuse, specular) ...
    // Modulate the material's colour (from the texture) with the light
    vec3 result = (ambient + diffuse) * texColor.rgb + specular;
    outColor = vec4(result, 1.0);
    ```
9. Run: Compile and run. Your cube should now be textured

#### Solution
So, for this exercise I had to update some functions that where already implemented on the Vulkan pipeline, in this case the DescriptorSetLayout and the DescriptorPool in order to add the uniform buffer and the texture sampler.

```cpp
void HelloTriangleApplication::createDescriptorSetLayout() {
    VkDescriptorSetLayoutBinding uboLayoutBinding{};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    // This is the new piece:
    VkDescriptorSetLayoutBinding samplerLayoutBinding{};
    samplerLayoutBinding.binding = 1;
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    std::array<VkDescriptorSetLayoutBinding, 2> bindings = { uboLayoutBinding, samplerLayoutBinding };
    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create descriptor set layout!");
    }
}
```

```cpp
void HelloTriangleApplication::createDescriptorPool() {
    std::array<VkDescriptorPoolSize, 2> poolSizes{};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

    if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create descriptor pool!");
    }
}
```

```cpp
void HelloTriangleApplication::createDescriptorSets() {
    std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    allocInfo.pSetLayouts = layouts.data();

    descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
    if (vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data()) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate descriptor sets!");
    }

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = uniformBuffers[i];
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(UniformBufferObject);

        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = textureImageView;
        imageInfo.sampler = textureSampler;

        std::array<VkWriteDescriptorSet, 2> descriptorWrites{};

        // (1) Uniform Buffer
        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = descriptorSets[i];
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo = &bufferInfo;

        // (2) Texture Sampler
        descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstSet = descriptorSets[i];
        descriptorWrites[1].dstBinding = 1;
        descriptorWrites[1].dstArrayElement = 0;
        descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].pImageInfo = &imageInfo;

        vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
    }
}
```

Then I added the createTexture, createTextureImageView and createTextureSampler functions to the initVulkan function in order to start the reading but before that I also had to change the sahders in order to pass the sampling of the image and grab the colors per UV.

```cpp
void HelloTriangleApplication::initVulkan() {
    createInstance();
    setupDebugMessenger();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
    createSwapChain();
    createImageViews();
    createDescriptorSetLayout();
    createGraphicsPipeline();
    createCommandPool();

    // loadModel();

    createTextureImage();
    createTextureImageView();
    createTextureSampler();

    // Create three cubes at dfferent positions
    std::vector<GeometryUtils::MeshData> meshes = {
        Geometry::CreateCube(1.0f, glm::vec3(0.0f, 0.0f, 0.0f)),   // Center cube
    };

    // Clear global vectors
    meshBuffers.clear();
    modelMatrices.clear();

    //Initial transforms for each cube
    modelMatrices.push_back(glm::mat4(1.0f)); // Center cube
```

This is why the final step was updating the shaders in order to pass the information and then grabbing the textureSampling.xyz into the final color rendering.

```glsl
#version 450

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec3 lightPos1;
    vec3 lightColor1;
    vec3 lightPos2;
    vec3 lightColor2;
    vec3 eyePos;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec2 inTexCoord; // Texture coordinate output

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 fragWorldPos;
layout(location = 2) out vec3 fragWorldNormal;
layout(location = 3) out vec2 fragTexCoord; // Texture coordinate output

void main() {
    // Calculate world position and normal usin ubo.model
    fragWorldPos = (ubo.model * vec4(inPosition, 1.0)).xyz;
    fragWorldNormal = mat3(transpose(inverse(ubo.model))) * inNormal;
    fragColor = inColor;
    fragTexCoord = inTexCoord; // Pass through texture coordinate;

    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0);
}
```

```glsl
#version 450

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec3 lightPos1;
    vec3 lightColor1;
    vec3 lightPos2;
    vec3 lightColor2;
    vec3 eyePos;
} ubo;

layout(push_constant) uniform MaterialPushConstant {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float shininess;
} material;

layout(binding = 1) uniform sampler2D texSampler;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragWorldPos;
layout(location = 2) in vec3 fragWorldNormal;
layout(location = 3) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

void main() {
    vec3 norm = normalize(fragWorldNormal);
    vec3 viewDir = normalize(ubo.eyePos - fragWorldPos);

    // // Sample the texture
    vec4 texColor = texture(texSampler, fragTexCoord);

    // // --- Lighting calculations (Phong model with two lights) ---
    vec3 ambient = material.ambient * texColor.rgb;

    // // Light 1
    vec3 lightDir1 = normalize(ubo.lightPos1 - fragWorldPos);
    float diff1 = max(dot(norm, lightDir1), 0.0);
    vec3 diffuse1 = material.diffuse * diff1 * ubo.lightColor1;

    vec3 reflectDir1 = reflect(-lightDir1, norm);
    float spec1 = pow(max(dot(viewDir, reflectDir1), 0.0), material.shininess);
    vec3 specular1 = material.specular * spec1 * ubo.lightColor1;

    // // Light 2
    vec3 lightDir2 = normalize(ubo.lightPos2 - fragWorldPos);
    float diff2 = max(dot(norm, lightDir2), 0.0);
    vec3 diffuse2 = material.diffuse * diff2 * ubo.lightColor2;

    vec3 reflectDir2 = reflect(-lightDir2, norm);
    float spec2 = pow(max(dot(viewDir, reflectDir2), 0.0), material.shininess);
    vec3 specular2 = material.specular * spec2 * ubo.lightColor2;

    vec3 diffuse = diffuse1 + diffuse2;
    vec3 specular = specular1 + specular2;

    // // Combine lighting with texture color
    vec3 result = (ambient + diffuse) * texColor.rgb + specular;
    outColor = vec4(result, 1.0);
}
```

- Test data
N/A

- Sample output
N/A

- Reflection

    - *Reflect on what you have learnt from this exercise.*
      I learnt what to change in order to grab the colors of the sampler per UV and apply it on the final rendering figure.

    - *Did you make any mistakes?*
      No.

    - *In what way has your knowledge improved?*
      It improved in the sense that now I know how to pass from the vertex shader into the fragment shader the sampling and the UV vec2.

- Questions

    - *Is there anything you would like to ask?*
      No.

### Exercise 4: A WOODEN CUBE

#### Question
Convert the provided wood.dds texture and convert it to jpg format using an online texture converter, for example, https://convert.guru/converter. the existing texture file texture.jpg with the newly converted wood.jpg.

In your implementation of per-fragment lighting from Lab 4, substitute the vertex colour input with the wood.jpg texture. This change will produce the intended visual effect as illustrated below.

#### Solution
I put the path of the wood jpg into the createTextureIMage and ran the program.

```cpp
void HelloTriangleApplication::createTextureImage() {
    int texWidth, texHeight, texChannels;
    stbi_uc* pixels = stbi_load("C:\\Users\\949145\\GitHub\\GraphicsLab5\\Vulkan(1_3)_Lab_05\\resources\\jpg\\wood.jpg", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    if (!pixels) {
        throw std::runtime_error("Failed to load texture image!");
}
```

Then I ran the program and this was the final output.

![Final render of the cube with wood texture mapping](<../markdown-resources/Complete Real Time Graphics Lab Book (1 - 8)/img5-4.png> "Final render of the cube with wood texture mapping")]

- Test data
N/A

- Sample output
A render of a cube with the wood.jpg as a texture mapped.

- Reflection

    - *Reflect on what you have learnt from this exercise.*
      I learnt how apply a texture to map into a mesh face, per UV points.

    - *Did you make any mistakes?*
      No.

    - *In what way has your knowledge improved?*
      In that now according to the meshes UV and the reading of the texture it can be applied into the Vulkan pipeline as long as the image can be read.

- Questions

    - *Is there anything you would like to ask?*
      No.

### Exercise 5: TEXTURE WRAPPING MODE

#### Question
Create a texture-mapped cube using the coin texture “Coin.jpg”, such that different faces of the cube have different number of coin patterns

#### Solution
In my current implementation, each face of the cube uses the same UV mapping (uvs[0] to uvs[3]), which means each face will show the entire texture once.

To have different numbers of coin patterns per face, you will need to adjust the UV coordinates for each face so that, for example, one face might show the texture tiled 2x2, another 3x3. To do this, you need to scale the UV coordinates for each face from the default
[0, 1] range to [0, 2]. This will cause the texture to repeat twice in both the U and V directions, resulting in 4 tiles per face.

```cpp
// UVs: bottom-left, bottom-right, top-right, top-left
glm::vec2 uvs[4] uvs = {
    {0.0f, 2.0f}, // Bottom-left
    {2.0f, 2.0f}, // Bottom-right
    {2.0f, 0.0f}, // Top-right
    {0.0f, 0.0f}  // Top-left
};
```

I also had to change the samplerInfo or more than less to confirm that it was using the `VK_SAMPLER_ADDRESS_MODE_REPEAT`.

```cpp
samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
```

```glsl
layout(binding = 0) in vec3 fragColor;
layout(binding = 1) in vec3 fragWorldPos;
layout(binding = 2) in vec3 fragWorldNormal;
layout(binding = 3) in vec2 fragTexCoord;
layout(binding = 0, binding = 1) uniform sampler2D texSampler;

layout(location = 0) out vec4 outColor;
```

![Final render of the cube with coin texture mapping](<../markdown-resources/Complete Real Time Graphics Lab Book (1 - 8)/img5-5.png> "Final render of the cube with coin texture mapping")]

- Test data
N/A

- Sample output
A render of a cube a tiled coin texture.

- Reflection

    - *Reflect on what you have learnt from this exercise.*
      I learnt about the tilling and depending on the offset and tilling one can change how many times per face can the UV be applied.

    - *Did you make any mistakes?*
      No.

    - *In what way has your knowledge improved?*
      In that now that the creation of the mesh is essential in order to also change the offset and tilling of the texture.

- Questions

    - *Is there anything you would like to ask?*
      No.

### Exercise 6: TEXTURE FILTERING TECHNIQUES

#### Question
Scale the cube geometry along the view direction to create the visual impression of a long,
straight road extending into the distance. This transformation aligns the object with the
camera's perspective, enhancing depth perception and mimicking the appearance of linear
structures such as highways or corridors.
Next, apply various texture filtering techniques to address common issues in texture
mapping— specifically, minification and magnification. Techniques such as nearest
neighbour, bilinear, bi-cubic, and anisotropic filtering can be used to mitigate aliasing and
blurring artifacts.
As you implement and compare these filtering methods, observe how each affects the
visual fidelity of the rendered image.
This exercise highlights the importance of texture sampling strategies in real-time
rendering and their impact on perceived image quality, especially under non-uniform
geometric transformations

#### Solution
This time I had to change the cube’s geometry along the view direction to create the visual impression of a long, straight road.

The easiest way was to scale the model matrix in the direction you want the "road" to extend.

Since the camera is looking down the negative Z axis by default (from (5,5,5) to (0,0,0)), you can scale the cube along the Z axis.

```cpp
UniformBufferObject ubo{};
glm::mat4 scale = glm::scale(glm::mat4(1.0f), glm::vec3(1.0f, 0.1f, 10.0f));
glm::mat4 rotate = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
ubo.model = rotate * scale;

ubo.view = glm::lookAt(glm::vec3(5.0f, 5.0f, 5.0f), glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
ubo.proj = glm::perspective(glm::radians(45.0f), swapChainExtent.width / (float)swapChainExtent.height, 0.1f, 100.0f);
ubo.proj[1][1] *= -1;
```

Then I Implemented and switched between texture filtering techniques (nearest, bilinear, anisotropic, etc.)

I controlled texture filtering in Vulkan by changing the `VkSamplerCreateInfo` settings in the `createTextureSampler()` function. This is how I can implement each filtering method:
1. Nearest-neighbor filtering: Set both magFilter and minFilter to `VK_FILTER_NEAREST`.
2. Bilinear filtering: Set both magFilter and minFilter to `VK_FILTER_LINEAR`
3. Anisotropic filtering: Set `anisotropyEnable = VK_TRUE` and maxAnisotropy to the device limit.
4. (Optional) Bi-cubic filtering: Vulkan does not natively support bi-cubic filtering via the sampler. This would require a custom shader implementation, which is more advanced and not typical for basic Vulkan setups.

![Different filters implementations](<../markdown-resources/Complete Real Time Graphics Lab Book (1 - 8)/img5-6-implementations.png> "Different filters implementations")]]

![Nearest-neighbor](<../markdown-resources/Complete Real Time Graphics Lab Book (1 - 8)/img5-6-nearest-neighbor.png> "Different filters implementations")]]

![Bilinear](<../markdown-resources/Complete Real Time Graphics Lab Book (1 - 8)/img5-6-bilinear.png> "Different filters implementations")]]

![Anisotropic filtering](<../markdown-resources/Complete Real Time Graphics Lab Book (1 - 8)/img5-6-anisotropic.png> "Different filters implementations")]]

- Test data
N/A

- Sample output
Three sets of rendering filtering.

- Reflection

    - *Reflect on what you have learnt from this exercise.*
      I get to see how the different filtering methods affect how the texture is rendered.

    - *Did you make any mistakes?*
      Yes. Push constants were not passing correctly.

    - *In what way has your knowledge improved?*
      In that depending on the situation I can change the filtering method and see how it affects the texturing of the image per the mesh’s face.

- Questions

    - *Is there anything you would like to ask?*
      No.

### Exercise 7: MULTIPLE TEXTURING

#### Question
Convert the provided tile.dds texture to jpg format and use it together with the coin texture to create the following effect.

Convert the provided tile.dds texture to jpg format and use it together with the coin texture to create the following effect. Notice that it's a cube again, not a long road. And if you need a description of the image is a cube with the tiles.jpg and above that texture is the coin.jpg as texture in a four tile per face. Like if the tiles.jpg would be the "background" so to speak,
and on top of it the coing.jpg . And there's depth applied so that the cube renders only with the outward faces.

Ok, read Lab_Tutorial_template.cpp and GeometryUtils.h and GeometryUtils.cpp. We are doing an exercise about mapping textures on cubes. We currently need to do this instruction:Create a texture-mapped cube using the coin texture “coin.jpg”, such that
different faces of the cube have different number of coin patterns. But let's go bit by bit of what needs to be done. So please after each answer ask me to go the next bit of the solution until done.

![Exercise 7 Example](<../markdown-resources/Complete Real Time Graphics Lab Book (1 - 8)/img5-7-example.png> "Exercise 7 Example")]]

#### Solution
For this one I had to use a trick, to render the image. I had to grab the coin and send it to a web page that will remove it’s background color, and transform it into a png, with transparency on.

```cpp
// Add to HelloTriangleApplication (private section)
VkImage tilesImage;
VkDeviceMemory tilesImageMemory;
VkImageView tilesImageView;
VkSampler tilesTextureSampler;
```

```cpp
/* Second texture functions */
void createTilesTextureImage();
void createTilesTextureImageView();
void createTilesTextureSampler();
```

Then I had to create new functions to process the second texture. And then I applied the changes into the different functions to process the second texture.

```cpp
void HelloTriangleApplication::createTilesTextureImage() {
    int texWidth, texHeight, texChannels;
    stbi_uc* pixels = stbi_load("C:\\Users\\949145\\GitHub\\GraphicsLab5\\Vulkan(1_3)_Lab_05\\resources\\jpg\\Tiles.jpg", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    if (!pixels) {
        throw std::runtime_error("Failed to load tiles texture image!");
    }

    VkDeviceSize imageSize = texWidth * texHeight * 4;

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
        stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data);
    memcpy(data, pixels, static_cast<size_t>(imageSize));
    vkUnmapMemory(device, stagingBufferMemory);

    stbi_image_free(pixels);

    createImage(texWidth, texHeight, VK_FORMAT_R8G8B8A8_SRGB, 
        VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, tilesImage, tilesImageMemory);

    transitionImageLayout(tilesImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);
    copyBufferToImage(stagingBuffer, tilesImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
    transitionImageLayout(tilesImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);

    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
}

void HelloTriangleApplication::createTilesTextureImageView() {
    tilesImageView = createImageView(tilesImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);
}

void HelloTriangleApplication::createTilesTextureSampler() {
    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(physicalDevice, &properties);

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

    if (vkCreateSampler(device, &samplerInfo, nullptr, &tilesSampler) != VK_SUCCESS) {
        throw std::runtime_error("failed to create tiles texture sampler!");
    }
}
```

I implemented the changes to the new functions:
- createTilesTextureImage.
- createTilesTextureImageView
- createTilesTextureSameple

Then I just had to add them to the initVulkan() function. But I also had to change to the descriptorSet to have a new entry in the array for the second image. After applying the coin image, I set the descriptor info for the tiles image.

```cpp
void HelloTriangleApplication::createDescriptorSets() {
    std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    allocInfo.pSetLayouts = layouts.data();

    descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
    if (vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data()) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate descriptor sets!");
    }

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = uniformBuffers[i];
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(UniformBufferObject);

        VkDescriptorImageInfo coinImageInfo{};
        coinImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        coinImageInfo.imageView = textureImageView;
        coinImageInfo.sampler = textureSampler;

        VkDescriptorImageInfo tilesImageInfo{};
        tilesImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        tilesImageInfo.imageView = tilesImageView;
        tilesImageInfo.sampler = tilesSampler;

        std::array<VkWriteDescriptorSet, 3> descriptorWrites{};

        // (1) Uniform Buffer
        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = descriptorSets[i];
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo = &bufferInfo;

        // (2) Coin Texture Sampler
        descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstSet = descriptorSets[i];
        descriptorWrites[1].dstBinding = 1;
        descriptorWrites[1].dstArrayElement = 0;
        descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].pImageInfo = &coinImageInfo;

        // (3) Tiles Texture Sampler
        descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[2].dstSet = descriptorSets[i];
        descriptorWrites[2].dstBinding = 2;
        descriptorWrites[2].dstArrayElement = 0;
        descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[2].descriptorCount = 1;
        descriptorWrites[2].pImageInfo = &tilesImageInfo;

        vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
    }
}
```

```glsl
#version 450

layout(set = 0, binding = 1) uniform sampler2D coinTex;
layout(set = 0, binding = 2) uniform sampler2D tilesTex;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragWorldPos;
layout(location = 2) in vec3 fragWorldNormal;
layout(location = 3) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

void main() {
    // Sample the background tiles texture
    vec4 tilesSample = texture(tilesTex, fragTexCoord);

    // Sample the coin texture, tiled 2x2 per face
    vec2 coinUV = fragTexCoord * 2.0;
    vec4 coinSample = texture(coinTex, coinUV);

    // Alpha blend: coin over tiles
    float alpha = coinSample.a;
    vec3 blended = mix(tilesSample.rgb, coinSample.rgb, alpha);

    outColor = vec4(blended, 1.0);
}
```

```cpp
createTilesTextureImage();
createTilesTextureImageView();
createTilesTextureSampler();
```

I also had to change the fragment shader in order to pass the color of the second sampler in order to have them blended with a mix blend of the two rgb’s from the two textures. At then end I got the cube with the tiles of the coins and below that the tiles texture.

![Final render of the cube with coin and tiles texture mapping](<../markdown-resources/Complete Real Time Graphics Lab Book (1 - 8)/img5-7.png> "Final render of the cube with coin and tiles texture mapping")]]

- Test data
N/A

- Sample output
A render of a cube with the coins and the tiles.

- Reflection

    - *Reflect on what you have learnt from this exercise.*
      I had to edit the assets directly to render them correctly. In a sense I had to think outside the box to achieve the final result.

    - *Did you make any mistakes?*
      No.

    - *In what way has your knowledge improved?*
      In that when blending two textures you can mixed them in the fragment to render the two of them. I also tried to add depth to see if I can fix my cube.

- Questions

    - *Is there anything you would like to ask?*
      No.

### Exercise 8: AN OPEN BOX

#### Question
Create the following open box effects using the wood.jpg and rock.jpg textures.

![Exercise 8 Example](<../markdown-resources/Complete Real Time Graphics Lab Book (1 - 8)/img5-8-example.png> "Exercise 8 Example")]]

#### Solution
For this one, I had to fix an issue with the creation of the mesh. I had to change the createCube function in order to have multiple faces, but that was not the final solution, but I used a bool value to determine if it stays full wood or stone.

```cpp
/* OPEN BOX CUBE */

std::vector<Face> faces = {
    // +X
    { { 1, 0, 0 }, { 1, 0, 0 }, { {hs, -hs, -hs}, {hs, -hs, hs}, {hs, hs, hs}, {hs, hs, -hs} } },
    // -X
    { { -1, 0, 0 }, { 0, 1, 0 }, { {-hs, -hs, hs}, {-hs, -hs, -hs}, {-hs, hs, -hs}, {-hs, hs, hs} } },
    // +Y
    { { 0, 1, 0 }, { 0, 0, 1 }, { {-hs, hs, -hs}, {-hs, hs, hs}, {hs, hs, hs}, {hs, hs, -hs} } },
    // -Y
    { { 0, -1, 0 }, { 1, 1, 0 }, { {-hs, -hs, hs}, {hs, -hs, hs}, {hs, -hs, -hs}, {-hs, -hs, -hs} } },
    // +Z
    { { 0, 0, 1 }, { 0, 1, 1 }, { {-hs, -hs, hs}, {hs, -hs, hs}, {hs, hs, hs}, {-hs, hs, hs} } },
    // -Z
    { { 0, 0, -1 }, { 1, 0, 1 }, { {hs, -hs, -hs}, {-hs, -hs, -hs}, {-hs, hs, -hs}, {hs, hs, -hs} } },
};

glm::vec2 tex[4] = {
    {0.0f, 0.0f},
    {1.0f, 0.0f},
    {1.0f, 1.0f},
    {0.0f, 1.0f}
};

for (const auto& f : faces) {
    // Outward face
    uint32_t startIndex = static_cast<uint32_t>(mesh.vertices.size());
    for (int i = 0; i < 4; i++) {
        GeometryVertex v{};
        v.pos = f.v[i] + offset;
        v.color = f.color;
        v.normal = f.normal;
        v.texCoord = tex[i];
        v.faceType = isWoodOnly;
        mesh.vertices.push_back(v);
    }
    mesh.indices.push_back(startIndex + 0);
    mesh.indices.push_back(startIndex + 1);
    mesh.indices.push_back(startIndex + 2);
    mesh.indices.push_back(startIndex + 2);
    mesh.indices.push_back(startIndex + 3);
    mesh.indices.push_back(startIndex + 0);

    // Inward face - reverse winding, invert normal
    startIndex = static_cast<uint32_t>(mesh.vertices.size());
    for (int i = 0; i < 4; i++) {
        GeometryVertex v{};
        v.pos = f.v[i] + offset;
        v.color = f.color;
        v.normal = -f.normal; // Invert normal
        v.texCoord = tex[i];
        v.faceType = isWoodOnly;
        mesh.vertices.push_back(v);
    }
    // Reverse winding order for inward face
    mesh.indices.push_back(startIndex + 0);
    mesh.indices.push_back(startIndex + 3);
    mesh.indices.push_back(startIndex + 2);
    mesh.indices.push_back(startIndex + 2);
    mesh.indices.push_back(startIndex + 1);
    mesh.indices.push_back(startIndex + 0);
}

return mesh;
```

```cpp
struct GeometryVertex {
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec3 normal;
    glm::vec2 texCoord;
    int faceType;

    static VkVertexInputBindingDescription getBindingDescription() {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(GeometryVertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        return bindingDescription;
    }

    static std::array<VkVertexInputAttributeDescription, 5> getAttributeDescriptions() {
        std::array<VkVertexInputAttributeDescription, 5> attributeDescriptions{};
        attributeDescriptions[0] = { 0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(GeometryVertex, pos) };
        attributeDescriptions[1] = { 1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(GeometryVertex, color) };
        attributeDescriptions[2] = { 2, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(GeometryVertex, normal) };
        attributeDescriptions[3] = { 3, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(GeometryVertex, texCoord) };
        attributeDescriptions[4] = { 4, 0, VK_FORMAT_R32_SINT, offsetof(GeometryVertex, faceType) };
        return attributeDescriptions;
    }
};
```

```cpp
for (const auto& mesh : meshes){
    std::vector<GeometryVertex> verts;
    for (const auto& gv : mesh.vertices) {
        verts.push_back(GeometryVertex{ gv.pos, gv.color, gv.texCoord, gv.faceType });
```

I also had to change the fragment shader, this was actually the final soultion in whicih using the bool gl_FrontFacing I determined if the face was inside or outside the cube in order to render them, then I used the passing bool to deterine if it was full wood or not.

```glsl
#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec2 fragTexCoord;
layout(location = 3) flat in int fragFaceType;

layout(set = 0, binding = 1) uniform sampler2D rocksSampler;
layout(set = 0, binding = 2) uniform sampler2D woodSampler;

layout(location = 0) out vec4 outColor;

void main() {
    vec4 texColor;
    
    // Select texture based on whether the face is front-facing or back-facing
    if (gl_FrontFacing) {
        texColor = texture(rocksSampler, fragTexCoord);
    } else {
        texColor = texture(woodSampler, fragTexCoord);
    }

    // Override texture if a specific face type is identified
    if(fragFaceType == 1) {
        texColor = texture(woodSampler, fragTexCoord);
    }

    // You can combine texColor with lighting and other effects as needed
    outColor = texColor;
}
```

At the end I got the final render required.

![Final render of the open box with wood and rock texture mapping](<../markdown-resources/Complete Real Time Graphics Lab Book (1 - 8)/img5-8.png> "Final render of the open box with wood and rock texture mapping")]]

- Test data
N/A

- Sample output
A render of two cubes, one full in wood the other with its outward faces in stone.

- Reflection

    - *Reflect on what you have learnt from this exercise.*
      This one was difficult because I had to figure out what was wrong in the generation of the cube and I was focusing a lot on the mesh and not on other solutions.

    - *Did you make any mistakes?*
      Yes. At the start I noticed that the cube was not being made correctly when I tried to extract the top face.

      ![Wrong cube generation](<../markdown-resources/Complete Real Time Graphics Lab Book (1 - 8)/img5-8-wrong-cube.png> "Wrong cube generation")]

    - *In what way has your knowledge improved?*
      In that when dealing with the creation of the mesh is not the proper solution, but sometimes the answer relies on the shaders.

- Questions

    - *Is there anything you would like to ask?*
      No.

### Final Reflection
This one was quite amusing because I learnt how to map the texture and grab the files, I had to think outies the box in order to achieve the requirements of the exercises.

---

## Lab book Chapter 6: Bump mapping

In the previous lab, we added realism by wrapping 2D images (textures) onto our 3D models. However, the surfaces are still perfectly flat, and the lighting doesn't interact with
any small-scale bumps or grooves. Bump Mapping is a powerful set of techniques that solves this by faking fine-grained surface detail without adding any extra geometry.

This lab will start from Normal Mapping, the most common form of bump mapping and then further explore other bump mapping methods, such as height mapping, procedural
bump mapping and ray- tracing based parallax mapping. We will simulate intricate surface details like bumps, cracks, and rivets by replacing the smooth vertex normal with a detailed
normal calculated from a texture carrying special information defined based on tangent space. This will require us to learn about a new coordinate system—Tangent Space—which is the foundation for most advanced shading techniques.

### Exercise 1: PREPARING FOR TANGENT SPACE

#### Question
1. Goal: Update the application to support tangent and binormal vectors, which are required for normal mapping.
2. Implementation:
    1. Modify Vertex Struct: Add tangent and binormal to your C++ Vertex struct.
       ```cpp
        struct Vertex {
            glm::vec3 pos;
            glm::vec3 color;
            glm::vec3 normal;
            glm::vec2 texCoord;
            glm::vec3 tangent; // New
            glm::vec3 binormal; // New
        };
        ```
2. Update Vertex Input Description: Add two new `VkVertexInputAttributeDescription` entries for tangent and binormal.
Remember to update all location and offset values.
3. Update Vertex Data: Calculating tangents for a complex mesh is hard, but for our simple quad, it's easy. Update your Quad_vertices (or equivalent) with
the TBN vectors. For a flat quad on the XY plane with Normal = (0,0,1), the Tangent will point along the x-axis and the Binormal along the y-axis.
```cpp
const std::vector<Vertex> Quad_vertices = {
    // pos color normal texCoord tangent binormal
    {{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}},
    {{ 0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}},
    ... ...
    ... ...
};
```
4. Get Textures: In addition to a colour texture, you need a normal map for normal mapping, a height map for height mapping and parallax mapping. Normal maps are typically purple -ish images.

#### Solution
For this solution I had to adapt the code as requested by the previous explanation, so first I had to change the MeshData struct in order to add the tangent and binormal glm::vec3.
And then change the `getAttributeDescriptions` function to include the return array the tangent and binormal.

```cpp
struct GeometryVertex {
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec3 normal;
    glm::vec2 texCoord;
    glm::vec3 tangent;
    glm::vec3 binormal;

    static VkVertexInputBindingDescription getBindingDescription() {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(GeometryVertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        return bindingDescription;
    }

    static std::array<VkVertexInputAttributeDescription, 6> getAttributeDescriptions() {
        std::array<VkVertexInputAttributeDescription, 6> attributeDescriptions{};
        attributeDescriptions[0] = { 0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(GeometryVertex, pos) };
        attributeDescriptions[1] = { 1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(GeometryVertex, color) };
        attributeDescriptions[2] = { 2, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(GeometryVertex, normal) };
        attributeDescriptions[3] = { 3, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(GeometryVertex, texCoord) };
        attributeDescriptions[4] = { 4, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(GeometryVertex, tangent) };
        attributeDescriptions[5] = { 5, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(GeometryVertex, binormal) };
        return attributeDescriptions;
    }

    static VkVertexInputAttributeDescription getTexCoordAttributeDescription() {
        VkVertexInputAttributeDescription texCoordDescription{};
        texCoordDescription.binding = 0;
        texCoordDescription.location = 3;
        texCoordDescription.format = VK_FORMAT_R32G32_SFLOAT;
        texCoordDescription.offset = offsetof(GeometryVertex, texCoord);
        return texCoordDescription;
    }
};
```

Then because the exercise requested a quad and I didn’t have a function for the creation of one I just added the createQuad function.

```cpp
namespace GeometryUtils {

    struct MeshData {
        std::vector<GeometryVertex> vertices;
        std::vector<uint32_t> indices;
    };

    MeshData CreateCube(float size, const glm::vec3& offset);
    MeshData CreateGrid(int width, int depth, const glm::vec3& offset);
    MeshData CreateTerrain(int width, int depth, float scale, const glm::vec3& offset);
    MeshData CreateCylinder(float radius, float height, int segments, const glm::vec3& offset);
    MeshData CreateSphere(float radius, int stacks, int slices, const glm::vec3& offset);
    MeshData CreateQuad(float width, float height, const glm::vec3& offset);

}
```

Then of course I had to implement the createQuad function with its tangent and binormal’s information.

```cpp
GeometryUtils::MeshData GeometryUtils::CreateQuad(float width, float height, const glm::vec3& offset) {
    MeshData mesh;

    float hw = width * 0.5f;
    float hh = height * 0.5f;

    // Vertex positions and texture coordinates
    glm::vec3 positions[4] = {
        glm::vec3(-hw, -hh, 0.0f) + offset, // bottom-left
        glm::vec3(hw, -hh, 0.0f) + offset,  // bottom-right
        glm::vec3(hw, hh, 0.0f) + offset,   // top-right
        glm::vec3(-hw, hh, 0.0f) + offset   // top-left
    };

    glm::vec2 texCoords[4] = {
        glm::vec2(0, 1),
        glm::vec2(1, 1),
        glm::vec2(1, 0),
        glm::vec2(0, 0)
    };

    glm::vec3 colors[4] = {
        glm::vec3(1, 0, 0),
        glm::vec3(0, 1, 0),
        glm::vec3(0, 0, 1),
        glm::vec3(1, 1, 1)
    };

    // Calculate tangent and binormal using the first triangle (0,1,2)
    glm::vec3 edge1 = positions[1] - positions[0];
    glm::vec3 edge2 = positions[2] - positions[0];
    glm::vec2 deltaUV1 = texCoords[1] - texCoords[0];
    glm::vec2 deltaUV2 = texCoords[2] - texCoords[0];

    float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

    glm::vec3 tangent = f * (deltaUV2.y * edge1 - deltaUV1.y * edge2);
    glm::vec3 binormal = f * (-deltaUV2.x * edge1 + deltaUV1.x * edge2);

    tangent = glm::normalize(tangent);
    binormal = glm::normalize(binormal);
    glm::vec3 normal = glm::normalize(glm::cross(tangent, binormal));

    // Assign to all vertices (for a flat quad, all are the same)
    for (int i = 0; i < 4; ++i) {
        mesh.vertices.push_back(GeometryVertex{
            positions[i],
            colors[i],
            normal,
            texCoords[i],
            tangent,
            binormal
        });
    }

    // Indices for two triangles
    mesh.indices = { 0, 1, 2, 2, 3, 0 };

    return mesh;
}
```

- Test data
N/A

- Sample output
No final render, just implementation of the functions.

- Reflection

    - *Reflect on what you have learnt from this exercise.*
      Because we are now talking about bumping maps we need to consider tangent space and its influence on the normal worlpsace to create the bump illusion.

    - *Did you make any mistakes?*
      No.

    - *In what way has your knowledge improved?*
      In that it’s important to offer the information via the mesh data of the tangent and the binormals in order to render the bumping correctly.

- Questions

    - *Is there anything you would like to ask?*
      No.

### Exercise 2: NORMAL MAPPING IMPLEMENTATION

#### Question
1. Goal: Implement the full normal mapping pipeline to render a bumpy-looking surface.
2. Implementation:
1. C++: Create a set of VkImage/VkImageView/VkSampler for the colour texture and the normal texture map.
2. Vertex Shader (shader.vert):
    1. Make a copy of the vertex shader for your per-fragment lighting implemented in Lab 5.
    2. Edit the vertex shader by adding new inputs for inTangent and inBinormal.
        ```glsl
        ... ...
        ... ...
        layout(location = 4) in vec3 inTangent;
        layout(location = 5) in vec3 inBinormal;
        layout(location = 0) out ... ...
        ... ...
        layout(location = 3) out vec2 fragTexCoord;
        layout(location = 4) out vec3 fragLightPos_tangent;
        layout(location = 5) out vec3 fragViewPos_tangent;
        layout(location = 6) out vec3 fragPos_tangent;
        ```
3. In main(), calculate the TBN matrix and transform the directions:
```glsl
void main() {
    // ... (standard MVP for gl_Position)
    ... ...
    ... ...
    // Create the TBN matrix (transforms from world-space to tangent space)
    Mat4 ModelMatrix_TInv= transpose(inverse(ubo.model);
    vec3 T = normalize(mat3(ModelMatrix_TInv) * inTangent);
    vec3 B = normalize(mat3(ModelMatrix_TInv) * inBinormal);
    vec3 N = normalize(mat3(ModelMatrix_TInv) * inNormal);
    mat3 TBN = transpose(mat3(T, B, N)); // Use transpose to invert
    // Get world-space light and view positions
    vec3 lightPos_world = ubo.lightPos;
    vec3 viewPos_world = ubo.viewPos;
    vec3 fragPos_world = (ubo.model * vec4(inPosition, 1.0)).xyz;
    // Transform light and view POSITIONS to tangent space
    fragLightPos_tangent = TBN * lightPos_world;
    fragViewPos_tangent = TBN * viewPos_world;
    fragPos_tangent =TBN *fragPos_world;
}
```
3. Fragment Shader (shader.frag):
    1. Add the new in variables and the new sampler2D for the normal map.
       ```glsl
       layout(binding = 1) uniform sampler2D colSampler;
       layout(binding = 2) uniform sampler2D normalSampler;
       layout(location = 3) in vec2 fragTexCoord;
       layout(location = 4) in vec3 fragLightPos_tangent;
       layout(location = 5) in vec3 fragViewPos_tangent;
       ```
    2. In main(), perform all lighting in tangent space.
       ```glsl
        void main() {
            // Get base color
            vec3 albedo = texture(colSampler, fragTexCoord).rgb;
            // Get normal from normal map
            vec3 N_tangent = texture(normalSampler, fragTexCoord).rgb;
            N_tangent = normalize(N_tangent * 2.0 - 1.0);
            // Implement lighting in tangent space based on normal
            N_tangent
            vec3 ambient = ... ...;
            vec3 diffuse = ... ...;
            vec3 specular = ... ...;
            vec3 result = ambient + diffuse + specular;
            outColor = vec4(result, 1.0);
        }
        ```
3. Expected Outcome: The rendered quad (or cube) will now appear with a bumpy surface. If you are using the provided coin textures, you should create a visual effect
shown in the left figure. Consider how to modify the normal to make the bump looks more significant shown in the right figure.

![Normal Mapping Example](<../markdown-resources/Complete Real Time Graphics Lab Book (1 - 8)/img6-2-example.png> "Normal Mapping Example")]]

#### Solution
For this exercise, I still needed to follow the instructions of the exercise. First I wanted to change shaders because now we have to use the tangent and binormal as well as the sampled normal map.
First I changed the vertex shader, including the normal sampler as and out for the fragment shader as well as the matrix inversions for the normal, tangent and binormal. Then, in the
latter, I did the tangent space calculations with the normal map and adding it to the light calculations. I also added a bump strength value and increase it to show better results.

```glsl
#version 450

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec3 lightPos1;
    vec3 lightColor1;
    vec3 lightPos2;
    vec3 lightColor2;
    vec3 eyePos;
} ubo;

layout(push_constant) uniform MaterialPushConstant {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float shininess;
} material;

// Texture and normal map samplers
layout(set = 0, binding = 1) uniform sampler2D colSampler;
layout(set = 0, binding = 2) uniform sampler2D normalSampler;

// Inputs from vertex shader
layout(location = 3) in vec2 fragTexCoord;
layout(location = 4) in vec3 fragLightPos_tangent;
layout(location = 5) in vec3 fragViewPos_tangent;
layout(location = 6) in vec3 fragPos_tangent;

layout(location = 0) out vec4 outColor;

void main() {
    float bumpStrength = 3.0;

    // Sample base color
    vec3 albedo = texture(colSampler, fragTexCoord).rgb;

    // Sample and decode normal from normal map (tangent space)
    vec3 N_tangent = texture(normalSampler, fragTexCoord).rgb;
    N_tangent = N_tangent * 2.0 - 1.0;
    N_tangent.xy *= bumpStrength;
    N_tangent = normalize(N_tangent);

    // Tangent-space light and view directions
    vec3 lightDir = normalize(fragLightPos_tangent - fragPos_tangent);
    vec3 viewDir = normalize(fragViewPos_tangent - fragPos_tangent);

    // Ambient
    vec3 ambient = material.ambient * albedo;

    // Diffuse
    float diff = max(dot(N_tangent, lightDir), 0.0);
    vec3 diffuse = material.diffuse * diff * albedo;

    // Specular (Phong)
    vec3 reflectDir = reflect(-lightDir, N_tangent);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    vec3 specular = material.specular * spec;

    // Combine
    vec3 result = ambient * (diffuse + specular);
    outColor = vec4(diffuse, 1.0);
}
```

```glsl
#version 450

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec3 lightPos1;
    vec3 lightColor1;
    vec3 lightPos2;
    vec3 lightColor2;
    vec3 eyePos;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec2 inTexCoord;
layout(location = 4) in vec3 inTangent;
layout(location = 5) in vec3 inBinormal;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 fragWorldPos;
layout(location = 2) out vec3 fragWorldNormal;
layout(location = 3) out vec2 fragTexCoord;
layout(location = 4) out vec3 fragLightPos_tangent;
layout(location = 5) out vec3 fragViewPos_tangent;
layout(location = 6) out vec3 fragPos_tangent;

void main() {
    // Standard MVP transform
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0);

    // World-space outputs
    fragWorldPos = (ubo.model * vec4(inPosition, 1.0)).xyz;
    fragWorldNormal = mat3(transpose(inverse(ubo.model))) * inNormal;
    fragColor = inColor;
    fragTexCoord = inTexCoord;

    // TBN matrix calculation (model-space to world-space, then to tangent-space)
    mat3 ModelMatrix_TInv = mat3(transpose(inverse(ubo.model)));
    vec3 T = normalize(ModelMatrix_TInv * inTangent);
    vec3 B = normalize(ModelMatrix_TInv * inBinormal);
    vec3 N = normalize(ModelMatrix_TInv * inNormal);
    mat3 TBN = transpose(mat3(T, B, N)); // Invert to go from world to tangent space

    // Use first light for demonstration (can be changed as needed)
    vec3 lightPos_world = ubo.lightPos1;
    vec3 viewPos_world = ubo.eyePos;
    vec3 fragPos_world = fragWorldPos;

    // Transform positions to tangent space
    fragLightPos_tangent = TBN * lightPos_world;
    fragViewPos_tangent = TBN * viewPos_world;
    fragPos_tangent = TBN * fragPos_world;
}
```

After that I created the class member variables for the normal map and its functions just as I did with the texture in the previous exercise. I pretty much copied the functions
implementation too changing the variables with the newly created ones for the normal map.

```cpp
// --- Normal Mapping Members ---
VkImage normalMapImage;
VlDeviceMemory normalMapImageMemory;
VkImageView normalMapImageView;
VkSampler normalMapSampler;
```

```cpp
/* Normal Mapping Members */
void createNormalMapImage();
void createNormalMapImageView();
void createNormalMapSampler();
```

```cpp
// --- Normal Map Implementations ---
void HelloTriangleApplication::createNormalMapImage() {
    int texWidth, texHeight, texChannels;
    stbi_uc* pixels = stbi_load("C:\\Users\\949145\\GitHub\\GraphicsLab6\\resources\\bmp\\rockNormal.bmp", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

    if (!pixels) {
        throw std::runtime_error("Failed to load normal map image!");
    }

    VkDeviceSize imageSize = texWidth * texHeight * 4;

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;

    createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingBuffer,
        stagingBufferMemory
    );

    void* data;
    vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data);
    memcpy(data, pixels, static_cast<size_t>(imageSize));
    vkUnmapMemory(device, stagingBufferMemory);

    stbi_image_free(pixels);

    createImage(texWidth, texHeight, VK_FORMAT_R8G8B8A8_SRGB,
        VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, normalMapImage, normalMapImageMemory
    );

    transitionImageLayout(normalMapImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);
    copyBufferToImage(stagingBuffer, normalMapImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
    transitionImageLayout(normalMapImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);

    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
}

void HelloTriangleApplication::createNormalMapImageView() {
    normalMapImageView = createImageView(normalMapImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);
}
```

```cpp
void HelloTriangleApplication::createNormalMapSampler() {
    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(physicalDevice, &properties);

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

    if (vkCreateSampler(device, &samplerInfo, nullptr, &normalMapSampler) != VK_SUCCESS) {
        throw std::runtime_error("failed to create normal map sampler!");
    }
}
```

Then on the createDescriptorLayout function I had to create VkDescriptorSetLayoutBinding for the new binding for the normal map.

```cpp
void HelloTriangleApplication::createDescriptorSetLayout() {
    VkDescriptorSetLayoutBinding uboLayoutBinding{};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutBinding colorSamplerLayoutBinding{};
    colorSamplerLayoutBinding.binding = 1;
    colorSamplerLayoutBinding.descriptorCount = 1;
    colorSamplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    colorSamplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutBinding normalSamplerLayoutBinding{};
    normalSamplerLayoutBinding.binding = 2;
    normalSamplerLayoutBinding.descriptorCount = 1;
    normalSamplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    normalSamplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    std::array<VkDescriptorSetLayoutBinding, 3> bindings = {
        uboLayoutBinding, colorSamplerLayoutBinding, normalSamplerLayoutBinding
    };

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create descriptor set layout!");
    }
}
```

And then on the createDescriptorPool I had to multiply the descriptorCount per 2. Because now it’s the texture and the normal map.

```cpp
void HelloTriangleApplication::createDescriptorPool() {
    std::array<VkDescriptorPoolSize, 2> poolSizes{};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) * 2; // For color and normal maps

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

    if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create descriptor pool!");
    }
}
```

Finally on the createDescriptorSets I had to add a new descriptorWrite and fill the information with the normal map information accordingly. And also its ImageInfo, andexpand to 3 the `VkWriteDecriptorSet`.

```cpp
void HelloTriangleApplication::createDescriptorSets() {
    std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    allocInfo.pSetLayouts = layouts.data();

    descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
    if (vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data()) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate descriptor sets!");
    }

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = uniformBuffers[i];
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(UniformBufferObject);

        VkDescriptorImageInfo colorImageInfo{};
        colorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        colorImageInfo.imageView = textureImageView;
        colorImageInfo.sampler = textureSampler;

        VkDescriptorImageInfo normalImageInfo{};
        normalImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        normalImageInfo.imageView = normalMapImageView;
        normalImageInfo.sampler = normalMapSampler;

        std::array<VkWriteDescriptorSet, 3> descriptorWrites{};

        // (1) Uniform Buffer
        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = descriptorSets[i];
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo = &bufferInfo;

        // (2) Color Texture Sampler
        descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstSet = descriptorSets[i];
        descriptorWrites[1].dstBinding = 1;
        descriptorWrites[1].dstArrayElement = 0;
        descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].pImageInfo = &colorImageInfo;

        // (3) Normal Map Sampler
        descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[2].dstSet = descriptorSets[i];
        descriptorWrites[2].dstBinding = 2;
        descriptorWrites[2].dstArrayElement = 0;
        descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[2].descriptorCount = 1;
        descriptorWrites[2].pImageInfo = &normalImageInfo;

        vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
    }
}
```

![Final render of the quad with normal mapping](<../markdown-resources/Complete Real Time Graphics Lab Book (1 - 8)/img6-2.png> "Final render of the quad with normal mapping")]]]

- Test data
N/A

- Sample output
A render of a quad with texture and normal map implementation.

- Reflection

    - *Reflect on what you have learnt from this exercise.*
      That is because I have created a new binding, I must create all the necessary changes in the descriptors to pass correctly the normal. And about the fragment shader I must do the prober calculations to get the simulated height per the tangent.

    - *Did you make any mistakes?*
      Yes, the light position and the quad position didn’t reflect the bump.
      ![Wrong normal mapping render](<../markdown-resources/Complete Real Time Graphics Lab Book (1 - 8)/img6-2-wrong.png> "Wrong normal mapping render")]

    - *In what way has your knowledge improved?*
      In how to pretty much add the bindings to new samplers.

- Questions

    - *Is there anything you would like to ask?*
      No.

### Exercise 3: HEIGHT MAP BUMPY EFFECT

#### Question
1. Goal: Create a similar bumpy effect using only a grayscale height map.
2. Implementation:
a. C++: Load a height map (e.g., bricks_height.png) instead of a normal map.
b. Fragment Shader: You do not have a normal map to sample. Instead, you
must calculate the normal. This is commonly done using the dFdx and dFdy
functions, which compute derivatives ("slopes") of a value.
// modify the fragment shader for normal mapping by calculating the normal
from the hight //map in the following way:
// Reconstruct the normal from the height map
float h = texture(heightSampler, inTexCoord).r;
// (1). Calculate derivatives of world position w.r.t. screen space
float dFx= dFdx(h);
float dFy= dFdy(h);
float bumpHeight = ... ; //a value between 0.1 and 1
vec3 Normal = normalize( vec3(-dFx, -dFy, bumpHeight));
// Perform lighting in WORLD space using this new ' Normal '
// ...
3. Expected Outcome: A bumpy surface appearance is created based on the height
map without using a normal map. If you are using the height map shown on the left,
you should be able to generate the visual effect shown on the right.

#### Solution
So I pretty much recycled the functions for the normal, just changed the name for “height”, but I did create new class members for the exclusive use for height maps
 
```cpp
// --- Height Mapping Members ---
VkImage heightMapImage;
VkDeviceMemory heightMapImageMemory;
VkImageView heightMapImageView;
VkSampler heightMapSampler;
```

Just as its functions declarations.

```cpp
/* Height Mapping Starts */
void createHeightMapImage();
void createHeightMapImageView();
void createHeightMapSampler();
```

The real difficult part was doing the changes to the fragment shader, because I have to grab the height map, I followed these steps:
- Grab the sample albedo.
- Sample the height.
- Compute the derivatives of height in screen space.
- Reconstruct normal in tangent space.
- Transform normal to world space using the TBN matrix (Tangent-binormal-normal).
- World-space light and view directions.
- Normalize directions
- Light process.

```glsl
#version 450

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec3 lightPos1; // Used for lighting
    vec3 lightColor1; // Used for lighting
    vec3 lightPos2;
    vec3 lightColor2;
    vec3 eyePos;
} ubo;

layout(push_constant) uniform MaterialPushConstant {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float shininess;
} material;

// Texture and height map samplers
layout(set = 0, binding = 1) uniform sampler2D colSampler;
layout(set = 0, binding = 2) uniform sampler2D heightSampler;

// Inputs from vertex shader (MUST match vertex shader locations)
layout(location = 0) in vec3 fragColor; // We aren't using this but keeping it for completeness
layout(location = 3) in vec2 fragTexCoord;

layout(location = 7) in vec3 fragT;
layout(location = 8) in vec3 fragB;
layout(location = 9) in vec3 fragN;
layout(location = 10) in vec3 fragWorldPos;

// Define TBN matrix (World Space to Tangent Space transform is TBN transpose)
// TBN is defined here for convenience, but is conceptually a varying mat3.
mat3 fragTBN = mat3(fragT, fragB, fragN);

layout(location = 0) out vec4 outColor;

void main() {
    // Sample base color (Albedo)
    vec3 albedo = texture(colSampler, fragTexCoord).rgb;

    // 1. Sample height from height map (use red channel)
    float h = texture(heightSampler, fragTexCoord).r;

    // 2. Compute derivatives of height in screen space
    float dFx = dFdx(h);
    float dFy = dFdy(h);

    vec3 N = normalize(vec3(-dFx, -dFy, 0.005)); // Original normal

    // World-space light and view directions
    vec3 lightDir = normalize(ubo.lightPos1 - fragWorldPos);
    vec3 viewDir = normalize(ubo.eyePos - fragWorldPos);

    // Ensure directions are normalized (important for dot product)
    vec3 N_world = normalize(N); // Note: In this specific snippet, N is used directly below

    // ---------------------------- Lighting Calculation ----------------------------

    // Ambient
    // Assumes lightColor1 applies to ambient as well, but usually a global setting is used.
    // We use the diffuse/specular light color as a multiplier.
    vec3 ambient = material.ambient * ubo.lightColor1 * albedo;

    // Diffuse (Lambertian)
    float diff = max(dot(N, lightDir), 0.0);
    vec3 diffuse = material.diffuse * diff * ubo.lightColor1 * albedo;

    // Specular (Phong)
    vec3 reflectDir = reflect(-lightDir, N);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    vec3 specular = material.specular * spec * ubo.lightColor1;

    // Combine
    vec3 result = ambient + diffuse + specular;
    outColor = vec4(N, 1.0);
}
```

For the vertex shader I just had to do the matrix calculations.

```glsl
#version 450

// ---------------------------- UBO ----------------------------
layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec3 lightPos1;
    vec3 lightColor1;
    vec3 lightPos2;
    vec3 lightColor2;
    vec3 eyePos;
} ubo;

// ---------------------------- Inputs (GeometryVertex struct) ----------------------------
// Ensure these locations match your C++ getAttributeDescriptions:
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec2 inTexCoord;
layout(location = 4) in vec3 inTangent;
layout(location = 5) in vec3 inBinormal;

// ---------------------------- Outputs (To Fragment Shader) ----------------------------
layout(location = 0) out vec3 fragColor;
layout(location = 3) out vec2 fragTexCoord;

// TBN Vectors and World Position
layout(location = 7) out vec3 fragT; // Tangent
layout(location = 8) out vec3 fragB; // Binormal
layout(location = 9) out vec3 fragN; // Normal
layout(location = 10) out vec3 fragWorldPos; // World Position

void main() {
    // Standard MVP transform
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0);

    // World-space position
    fragWorldPos = (ubo.model * vec4(inPosition, 1.0)).xyz;

    // Pass color and texcoord
    fragColor = inColor;
    fragTexCoord = inTexCoord;

    // TBN matrix calculation: Transform T, B, N from model-space to world-space
    // using the Inverse Transpose of the Model Matrix for correct normal transformation.
    mat3 ModelMatrix_TInv = mat3(transpose(inverse(ubo.model)));

    // Normalize and pass TBN vectors in world space
    fragT = normalize(ModelMatrix_TInv * inTangent);
    fragB = normalize(ModelMatrix_TInv * inBinormal);
    fragN = normalize(ModelMatrix_TInv * inNormal);
}
```

In order to test if everything was working I did outputted the normalized N to see the normal map that was forming on tangent space.

![Height map normal visualization](<../markdown-resources/Complete Real Time Graphics Lab Book (1 - 8)/img6-3-normals.png> "Height map normal visualization")]

After some battles with the light I got it rendered.

![Final render of the quad with height mapping](<../markdown-resources/Complete Real Time Graphics Lab Book (1 - 8)/img6-3.png> "Final render of the quad with height mapping")]]]

- Test data
N/A

- Sample output
A render of a Quad with height map bumping.

- Reflection

    - *Reflect on what you have learnt from this exercise.*
      I learnt about the process for grabbing a height map and implement it as a bump.

    - *Did you make any mistakes?*
      Yes on the createDescriptorLayout, with the UBO descriptor I didn’t put that it shall also be used on the fragment shader, thus the UBO information was not being used propertly. I needed to add these flags:

      ```cpp
      uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
      ```
      And
    
      ![Wrong height mapping render](<../markdown-resources/Complete Real Time Graphics Lab Book (1 - 8)/img6-3-wrong.png> "Wrong height mapping render")]

      ![Wrong final height mapping render](<../markdown-resources/Complete Real Time Graphics Lab Book (1 - 8)/img6-3-wrong2.png> "Wrong final height mapping render")]]

    - *In what way has your knowledge improved?*
      In it important for the binding to set the stageFlags correctly, I was stuck a lot of time due to not passing correctly the UBO information for the light

- Questions

    - *Is there anything you would like to ask?*
      No.

### Exercise 4: PROCEDURAL NORMAL MAPPING

#### Question
1. Goal: Create a bumpy or patterned effect (like waves, bumps) without using either normal map or height map, by calculating a normal procedurally.
2. Implementation:
    1. Fragment Shader: In your fragment shader, invent a normal. For example, the procedural bump effect shown in my lecture note shown below:
        ![Procedural normal mapping example](<../markdown-resources/Complete Real Time Graphics Lab Book (1 - 8)/img6-4-example.png> "Procedural normal mapping example")]
3. Expected Outcome: A strange, synthetic, but perfectly lit pattern on your object.

#### Solution
For this one I just commented out the calls for the height map and then I also commented on the descriptor, binding and calling, in order to not let any part of the height map to be included.

```cpp
createTextureSample();

// Height mapping setup
//createHeightMapImage();
//createHeightMapImageView();
//createHeightMapSampler();
```

```cpp
void HelloTriangleApplication::createDescriptorSetLayout() {
    VkDescriptorSetLayoutBinding uboLayoutBinding{};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutBinding colorSamplerLayoutBinding{};
    colorSamplerLayoutBinding.binding = 1;
    colorSamplerLayoutBinding.descriptorCount = 1;
    colorSamplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    colorSamplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    /*VkDescriptorSetLayoutBinding normalSamplerLayoutBinding{};
    normalSamplerLayoutBinding.binding = 2;
    normalSamplerLayoutBinding.descriptorCount = 1;
    normalSamplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    normalSamplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    std::array<VkDescriptorSetLayoutBinding, 3> bindings = {
        uboLayoutBinding, colorSamplerLayoutBinding, normalSamplerLayoutBinding
    };

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create descriptor set layout!");
    }*/

    // FOR EXERCISE 4: Use only 2 bindings: UBO and color sampler
    std::array<VkDescriptorSetLayoutBinding, 2> bindings = { uboLayoutBinding, colorSamplerLayoutBinding };

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size()); // Should be 2
    layoutInfo.pBindings = bindings.data();

    if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor set layout!");
    }
}
```

```cpp
void HelloTriangleApplication::createDescriptorSets() {
    std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    allocInfo.pSetLayouts = layouts.data();

    descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
    if (vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data()) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate descriptor sets!");
    }

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = uniformBuffers[i];
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(UniformBufferObject);

        VkDescriptorImageInfo colorImageInfo{};
        colorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        colorImageInfo.imageView = textureImageView;
        colorImageInfo.sampler = textureSampler;

        VkDescriptorImageInfo normalImageInfo{};
        normalImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        normalImageInfo.imageView = heightMapImageView;
        normalImageInfo.sampler = heightMapSampler;

        //std::array<VkWriteDescriptorSet, 3> descriptorWrites{};
        // FOR EXERCISE 4: Use only 2 descriptor writes: UBO and color sampler
        std::array<VkWriteDescriptorSet, 2> descriptorWrites{};

        // (1) Uniform Buffer
        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = descriptorSets[i];
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo = &bufferInfo;

        // (2) Color Texture Sampler
        descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstSet = descriptorSets[i];
        descriptorWrites[1].dstBinding = 1;
        descriptorWrites[1].dstArrayElement = 0;
        descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].pImageInfo = &colorImageInfo;

        // (3) Height Map Sampler
        /*
        descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[2].dstSet = descriptorSets[i];
        descriptorWrites[2].dstBinding = 2;
        descriptorWrites[2].dstArrayElement = 0;
        descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[2].descriptorCount = 1;
        descriptorWrites[2].pImageInfo = &normalImageInfo;*/

        vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()),
            descriptorWrites.data(), 0, nullptr);
    }
}
```

By calculating the vec3 N = normalize(vec3(-dFx, -dFy, 0.05)); we can get a bumpier effect more akin to the real world use if you were to sit in front. Then on the fragment shader I had to implement an algorithm to have per tiles (4x4 per face) a semi sphere and bump it. The following image explains each step:

```glsl
#version 450

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec3 lightPos1; // Used for lighting
    vec3 lightColor1; // Used for lighting
    vec3 lightPos2;
    vec3 lightColor2;
    vec3 eyePos;
} ubo;

layout(push_constant) uniform MaterialPushConstant {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float shininess;
} material;

// Texture and height map samplers
layout(set = 0, binding = 1) uniform sampler2D colSampler;
//layout(set = 0, binding = 2) uniform sampler2D heightSampler;

// Inputs from vertex shader (MUST match vertex shader locations)
layout(location = 0) in vec3 fragColor; // We aren't using this but keeping it for completeness
layout(location = 3) in vec2 fragTexCoord;

layout(location = 7) in vec3 fragT;
layout(location = 8) in vec3 fragB;
layout(location = 9) in vec3 fragN;
layout(location = 10) in vec3 fragWorldPos;

// Define TBN matrix (World Space to Tangent Space transform is TBN transpose)
// TBN is defined here for convenience, but is conceptually a varying mat3.
mat3 fragTBN = mat3(fragT, fragB, fragN);

layout(location = 0) out vec4 outColor;

void main() {
    // Sample base color
    vec3 albedo = texture(colSampler, fragTexCoord).rgb;

    // --- PROCEDURAL BUMP GENERATION ---

    // 1. Tiling: Multiplies UVs by 4.0 for a 4x4 grid of tiles
    float tiling = 4.0;
    vec2 uv = fragTexCoord * tiling;

    // 2. Local Coordinates: Get coordinate system local to the bump (0.0 to 1.0)
    // The fract() function repeats the pattern in each tile.
    vec2 localUV = fract(uv);

    // 3. Centering: Shift the origin to the center of the local tile (range [-0.5, 0.5])
    vec2 centeredUV = localUV - 0.5;

    // 4. Distance to center and Radius check
    // The distance 'r' from the center of the current bump (maximum 0.5)
    float r = length(centeredUV);
    float bump_radius = 0.45; // Slightly less than 0.5 to give space between bumps

    // 5. Height Function (Semisphere)
    float h = 0.0;
    if (r < bump_radius) {
        // Use a function that creates a dome shape: h = sqrt(radius^2 - r^2)
        // or a simple parabola for efficiency: h = max(0.0, 1.0 - (r / bump_radius)^2)
        float normalized_r = r / bump_radius;
        h = max(0.0, 1.0 - normalized_r * normalized_r); // Height is 1 at center, 0 at radius
    }

    // 6. Slope Calculation
    // Use the GLSL derivatives on the procedurally generated height 'h'
    float dFx = dFdx(h);
    float dFy = dFdy(h);

    // 7. Normalization of Original Normal (for reference)
    vec3 N = normalize(vec3(-dFx, -dFy, 0.05)); // Original normal

    // 8. Reconstruct Normal in Tangent Space
    float bumpiness = 10.0f; // Adjust this to control how steep the bumps are

    // --- Continue with Lighting Calculation using N ---
    // The rest of the lighting code remains the same.
    vec3 lightDir = normalize(ubo.lightPos1 - fragWorldPos);
    vec3 viewDir = normalize(ubo.eyePos - fragWorldPos);

    // Ambient
    vec3 ambient = material.ambient * ubo.lightColor1 * albedo;

    // Diffuse (Lambertian)
    float diff = max(dot(N, lightDir), 0.0);
    vec3 diffuse = material.diffuse * diff * ubo.lightColor1 * albedo;

    // Specular (Phong)
    vec3 reflectDir = reflect(-lightDir, N);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    vec3 specular = material.specular * spec * ubo.lightColor1;

    // Combine
    vec3 result = ambient + diffuse + specular;
    float diff1 = max(dot(N, normalize(ubo.lightPos1 - fragWorldPos)), 0.0);
    result = ambient * (diff1 + specular);
    outColor = vec4(result, 1.0);
}
```

![Procedural normal mapping explanation](<../markdown-resources/Complete Real Time Graphics Lab Book (1 - 8)/img6-4.png> "Procedural normal mapping explanation")]]

- Test data
N/A

- Sample output
A render of a quad with procedural normal mapping per tile.

- Reflection

    - *Reflect on what you have learnt from this exercise.*
      The fact that you can pretty much create bump mapping without any resource is amazing, if you’re good enough you can save a lot of resources.

    - *Did you make any mistakes?*
      Yes, by adding a calcaulation of normalization to the tangent I got a lesser result.

      ![Wrong procedural normal mapping render](<../markdown-resources/Complete Real Time Graphics Lab Book (1 - 8)/img6-4-wrong.png> "Wrong procedural normal mapping render")]

    - *In what way has your knowledge improved?*
      In that I had to understand the algorithm to create a procedural bump map.

- Questions

    - *Is there anything you would like to ask?*
      No.

### Exercise 5: RAY MARCHING -BASED PARALLAX MAPPING (OPTIONAL)

#### Question
1. Goal: Use a height map to create a more convincing 3D illusion by shifting texture coordinates.
2. Implementation:
    1. C++: Load a colour texture, a normal map, and a height map.
    2. Fragment Shader:
        1. This technique modifies the texture coordinates before you sample.
        2. Take the viewDir_tangent you calculated.
    3. Sample the height from the height map: float height = texture(heightSampler, fragTexCoord).r;    
    4. Displace the texture coordinates based on the height and view angle following the details provided in my lecture note.
       ... ...
    5. Use these displacedUVs to sample your color and normal maps to calculate light at each fragment.
3. Expected Outcome: A much more realistic 3D effect. As you rotate the object, the "higher" parts of the texture (e.g., bricks) will appear to move over and occlude the "lower" parts (e.g., mortar).

![Ray marching parallax mapping example](<../markdown-resources/Complete Real Time Graphics Lab Book (1 - 8)/img6-5-example.png> "Ray marching parallax mapping example")]
 
#### Solution
For this exercise, I had to grab what did for the height map and combine it with the normal map, thus I had to also change the descriptors and bindings in order to add the three resources

```cpp
void HelloTriangleApplication::createDescriptorSetLayout() {
    // Binding 0: Uniform Buffer (Vertex & Fragment)
    VkDescriptorSetLayoutBinding uboLayoutBinding{};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

    // Binding 1: Color Sampler (Fragment)
    VkDescriptorSetLayoutBinding colorSamplerLayoutBinding{};
    colorSamplerLayoutBinding.binding = 1;
    colorSamplerLayoutBinding.descriptorCount = 1;
    colorSamplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    colorSamplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    // Binding 2: Normal Sampler (Fragment)
    VkDescriptorSetLayoutBinding normalSamplerLayoutBinding{};
    normalSamplerLayoutBinding.binding = 2;
    normalSamplerLayoutBinding.descriptorCount = 1;
    normalSamplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    normalSamplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    // Binding 3: Height Sampler (Fragment)
    VkDescriptorSetLayoutBinding heightSamplerLayoutBinding{};
    heightSamplerLayoutBinding.binding = 3;
    heightSamplerLayoutBinding.descriptorCount = 1;
    heightSamplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    heightSamplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    std::array<VkDescriptorSetLayoutBinding, 4> bindings = {
        uboLayoutBinding, 
        colorSamplerLayoutBinding, 
        normalSamplerLayoutBinding, 
        heightSamplerLayoutBinding
    };

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create descriptor set layout!");
    }
}
```

```cpp
void HelloTriangleApplication::createDescriptorSets() {
    std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    allocInfo.pSetLayouts = layouts.data();

    descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
    if (vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data()) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate descriptor sets!");
    }

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = uniformBuffers[i];
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(UniformBufferObject);

        VkDescriptorImageInfo colorImageInfo{};
        colorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        colorImageInfo.imageView = textureImageView;
        colorImageInfo.sampler = textureSampler;

        VkDescriptorImageInfo normalImageInfo{};
        normalImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        normalImageInfo.imageView = normalMapImageView;
        normalImageInfo.sampler = normalMapSampler;

        VkDescriptorImageInfo heightImageInfo{};
        heightImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        heightImageInfo.imageView = heightMapImageView;
        heightImageInfo.sampler = heightMapSampler;

        std::array<VkWriteDescriptorSet, 4> descriptorWrites{};

        // (1) Uniform Buffer
        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = descriptorSets[i];
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo = &bufferInfo;

        // (2) Color Texture Sampler
        descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstSet = descriptorSets[i];
        descriptorWrites[1].dstBinding = 1;
        descriptorWrites[1].dstArrayElement = 0;
        descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].pImageInfo = &colorImageInfo;

        // (3) Normal Map Sampler
        descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[2].dstSet = descriptorSets[i];
        descriptorWrites[2].dstBinding = 2;
        descriptorWrites[2].dstArrayElement = 0;
        descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[2].descriptorCount = 1;
        descriptorWrites[2].pImageInfo = &normalImageInfo;

        // (4) Height Map Sampler
        descriptorWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[3].dstSet = descriptorSets[i];
        descriptorWrites[3].dstBinding = 3;
        descriptorWrites[3].dstArrayElement = 0;
        descriptorWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[3].descriptorCount = 1;
        descriptorWrites[3].pImageInfo = &heightImageInfo;

        vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
    }
}
```

```glsl
#version 450

// ---------------------------- UBO ----------------------------
layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec3 lightPos1;
    vec3 lightColor1;
    vec3 lightPos2;
    vec3 lightColor2;
    vec3 eyePos;
} ubo;

// ---------------------------- Inputs (GeometryVertex struct) ----------------------------
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec2 inTexCoord;
layout(location = 4) in vec3 inTangent;
layout(location = 5) in vec3 inBinormal;

// ---------------------------- Outputs (To Fragment Shader) ----------------------------
layout(location = 0) out vec3 fragColor;
layout(location = 3) out vec2 fragTexCoord;

layout(location = 7) out vec3 fragT; // Tangent (world space)
layout(location = 8) out vec3 fragB; // Binormal (world space)
layout(location = 9) out vec3 fragN; // Normal (world space)
layout(location = 10) out vec3 fragWorldPos; // World Position
layout(location = 11) out vec3 viewDir_tangent; // View direction in tangent space

void main() {
    // Standard MVP transform
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0);

    // World-space position
    fragWorldPos = (ubo.model * vec4(inPosition, 1.0)).xyz;

    // Pass color and texcoord
    fragColor = inColor;
    fragTexCoord = inTexCoord;

    // TBN matrix calculation: Transform T, B, N from model-space to world-space
    mat3 normalMatrix = mat3(transpose(inverse(ubo.model)));
    fragT = normalize(normalMatrix * inTangent);
    fragB = normalize(normalMatrix * inBinormal);
    fragN = normalize(normalMatrix * inNormal);

    // Calculate the World Space View Direction (from fragment to eye)
    vec3 V_world = normalize(ubo.eyePos - fragWorldPos);

    // World-to-Tangent-Space matrix is constructed from the TBN vectors:
    mat3 TBN = mat3(fragT, fragB, fragN);

    // Transform World View Direction into Tangent Space
    viewDir_tangent = TBN * V_world;
}
```

The most changed script was again the fragment shader because I had to implement the ray-marching based parallax algorithm. Displacing the UVs and then applying that new information to the albedo, bump and the lights.

```glsl
#version 450

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 model; mat4 view; mat4 proj;
    vec3 lightPos1; vec3 lightColor1; vec3 lightPos2; vec3 lightColor2;
    vec3 eyePos;
} ubo;

layout(push_constant) uniform MaterialPushConstant {
    vec3 ambient; vec3 diffuse; vec3 specular; float shininess;
} material;

layout(set = 0, binding = 1) uniform sampler2D colSampler;
layout(set = 0, binding = 2) uniform sampler2D normalSampler;
layout(set = 0, binding = 3) uniform sampler2D heightSampler;

layout(location = 0) in vec3 fragColor;
layout(location = 3) in vec2 fragTexCoord;
layout(location = 7) in vec3 fragT;
layout(location = 8) in vec3 fragB;
layout(location = 9) in vec3 fragN;
layout(location = 10) in vec3 fragWorldPos;
layout(location = 11) in vec3 viewDir_tangent;

layout(location = 0) out vec4 outColor;

void main() {
    // Parallax mapping parameters
    float heightScale = 0.05;

    // Sample height and compute parallax offset
    float height = texture(heightSampler, fragTexCoord).r;
    vec2 viewDirXY = normalize(viewDir_tangent.xy);
    vec2 parallaxOffset = (height - 0.5) * heightScale * viewDirXY;

    // Displaced UVs
    vec2 displacedUV = clamp(fragTexCoord + parallaxOffset, 0.0, 1.0);

    // Sample color and normal maps with displaced UVs
    vec3 albedo = texture(colSampler, displacedUV).rgb;
    vec3 normalSample = texture(normalSampler, displacedUV).rgb;
    vec3 N_tangent = normalize(normalSample * 2.0 - 1.0);

    mat3 TBN = mat3(fragT, fragB, fragN);
    vec3 N_world = normalize(TBN * N_tangent);

    // Lighting calculations (Phong)
    vec3 lightDir = normalize(ubo.lightPos1 - fragWorldPos);
    vec3 viewDir = normalize(ubo.eyePos - fragWorldPos);

    vec3 ambient = material.ambient * ubo.lightColor1 * albedo;
    float diff = max(dot(N_world, lightDir), 0.0);
    vec3 diffuse = material.diffuse * diff * ubo.lightColor1 * albedo;
    vec3 reflectDir = reflect(-lightDir, N_world);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    vec3 specular = material.specular * spec * ubo.lightColor1;

    outColor = vec4(ambient + diffuse + specular, 1.0);
}
```

```cpp
GeometryUtils::MeshData GeometryUtils::CreateCube(float size, const glm::vec3& offset) {
    MeshData mesh;

    float hs = size * 0.5f;

    // Define cube faces
    struct Face {
        glm::vec3 normal;
        glm::vec3 color;
        glm::vec3 v[4];
    };

    /* NORMAL CUBE */
    std::vector<Face> faces = {
        // +X
        { { 1, 0, 0 }, { 1, 0, 0 }, { {hs, -hs, -hs}, {hs, hs, -hs}, {hs, hs, hs}, {hs, -hs, hs} } },
        // -X
        { { -1, 0, 0 }, { 0, 1, 0 }, { {-hs, -hs, hs}, {-hs, hs, hs}, {-hs, hs, -hs}, {-hs, -hs, -hs} } },
        // +Y
        { { 0, 1, 0 }, { 0, 0, 1 }, { {-hs, hs, -hs}, {-hs, hs, hs}, {hs, hs, hs}, {hs, hs, -hs} } },
        // -Y
        { { 0, -1, 0 }, { 1, 1, 0 }, { {-hs, -hs, hs}, {-hs, -hs, -hs}, {hs, -hs, -hs}, {hs, -hs, hs} } },
        // +Z
        { { 0, 0, 1 }, { 0, 1, 1 }, { {-hs, -hs, hs}, {hs, -hs, hs}, {hs, hs, hs}, {-hs, hs, hs} } },
        // -Z
        { { 0, 0, -1 }, { 1, 0, 1 }, { {hs, -hs, -hs}, {-hs, -hs, -hs}, {-hs, hs, -hs}, {hs, hs, -hs} } },
    };

    glm::vec2 tex[4] = {
        {0.0f, 0.0f},
        {1.0f, 0.0f},
        {1.0f, 1.0f},
        {0.0f, 1.0f}
    };

    for (const auto& f : faces) {
        uint32_t startIndex = static_cast<uint32_t>(mesh.vertices.size());

        for (int i = 0; i < 4; i++) {
            GeometryVertex v{};
            v.pos = f.v[i];
            v.color = f.color;
            v.normal = f.normal;
            v.texCoord = tex[i];
            mesh.vertices.push_back(v);
        }

        // For each face in CreateCube: Calculate tangent space
        glm::vec3 edge1 = f.v[1] - f.v[0];
        glm::vec3 edge2 = f.v[2] - f.v[0];
        glm::vec2 deltaUV1 = tex[1] - tex[0];
        glm::vec2 deltaUV2 = tex[2] - tex[0];

        float f_denom = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);
        glm::vec3 tangent = f_denom * (deltaUV2.y * edge1 - deltaUV1.y * edge2);
        glm::vec3 binormal = f_denom * (-deltaUV2.x * edge1 + deltaUV1.x * edge2);

        tangent = glm::normalize(tangent);
        binormal = glm::normalize(binormal);

        // Assign tangent/binormal to all four vertices of this face
        for (int i = 0; i < 4; i++) {
            mesh.vertices[startIndex + i].tangent = tangent;
            mesh.vertices[startIndex + i].binormal = binormal;
        }

        // Two triangles per face
        mesh.indices.push_back(startIndex + 0);
        mesh.indices.push_back(startIndex + 1);
        mesh.indices.push_back(startIndex + 2);
        mesh.indices.push_back(startIndex + 2);
        mesh.indices.push_back(startIndex + 3);
        mesh.indices.push_back(startIndex + 0);
    }

    return mesh;
}
```

![Final render of the quad with ray marching parallax mapping](<../markdown-resources/Complete Real Time Graphics Lab Book (1 - 8)/img6-5.png> "Final render of the quad with ray marching parallax mapping")]]]

- Test data
N/A

- Sample output
A render of a cube with implementation of ray-marching based parallax.

- Reflection

    - *Reflect on what you have learnt from this exercise.*
      I learnt about the applience of normal maps na dheight maps and ray marching.

    - *Did you make any mistakes?*
      No.

    - *In what way has your knowledge improved?*
      Yes, I forgot to add tangent and binormal to the vertex data of the cubee.

- Questions

    - *Is there anything you would like to ask?*
      No.

### Final Reflection
I really liked this couple the exercises, because bump mapping is one of those “magic” things that can be done with maths and computer graphics, saving resources while making a game look good. Smoke and mirrors it’s what this is all about.

---

## Lab book Chapter 7: Cube mapping + Particle system

This lab introduces two powerful and essential techniques in modern 3D graphics: **Environment Mappin**g using a cubemap and **Particle Systems**.

First, we will use **Cube Mapping** to implement a "skybox," creating the illusion that our scene exists within a vast, 360-degree environment. We will then leverage this same environment map to create realistic, dynamic reflections and refractions on our objects, making them appear metallic or glass-like.

Second, we will build a sprite-based **Particle System** to render dynamic special effects like fire or smoke. This will involve "billboarding" (making 2D sprites always face the camera) and Alpha Blending to handle transparency.

Both of these techniques require precise control over the graphics pipeline, specifically when and how objects interact with the depth buffer and how their colours are blended.

### Exercise 1: IMPLEMENTING AND CONTROLLING THE DEPTH BUFFER

#### Question
This exercise is split into two parts. First, you will implement the depth buffer itself.
Second, you will use it to observe how depth testing and writing are controlled.
1. Goal: To create a depth buffer and then practically observe the effect of `depthTestEnable` and `depthWriteEnable`.
2. Implementation:
Before you can control depth, you must create the depth buffer's resources. This involves creating a VkImage and VkImageView that will be used as the depth attachment.

    - **C++ (Class Members)**: Add new Vulkan objects to your application class to manage the depth resources.
    ```cpp
    VkImage depthImage;
    VkDeviceMemory depthImageMemory;
    VkImageView depthImageView;
    VkFormat depthFormat;
    ```

    - **C++ (Implementation)**:
    1. Find Depth Format: Create a helper function to find a suitable depth format supported by the physical device.

      ![Instructions for depth buffer implementation](<../markdown-resources/Complete Real Time Graphics Lab Book (1 - 8)/img7-1-1.png> "Instructions for depth buffer implementation")]]

    2. **Create Depth Resources**: Create a new function createDepthResources() that will be called during initialization (and swapchain recreation). Remember to update all location and offset values.

      ![Instructions for depth buffer implementation](<../markdown-resources/Complete Real Time Graphics Lab Book (1 - 8)/img7-1-2.png> "Instructions for depth buffer implementation")]]

    3. **Update Pipeline**: In createGraphicsPipeline(), you must now configure the pipeline to use depth.

      ![Instructions for depth buffer implementation](<../markdown-resources/Complete Real Time Graphics Lab Book (1 - 8)/img7-1-3.png> "Instructions for depth buffer implementation")]]

    4. **Update Rendering**: In recordCommandBuffer(), you must now attach the depth buffer to the rendering process.

      ![Instructions for depth buffer implementation](<../markdown-resources/Complete Real Time Graphics Lab Book (1 - 8)/img7-1-4.png> "Instructions for depth buffer implementation")]]
      
    5. **Recreation**: Remember to call vkDestroyImageView, vkDestroyImage, and vkFreeMemory for the depth resources in cleanupSwapChain() and call createDepthResources() in recreateSwapChain()
3. Expected Outcome: Now you have a functioning depth buffer, you can experiment with its settings further. Draw a solid cube and observe how the depth setting may affect the visual result.

#### Solution
For this solution I had to create some class members for the depth testing, in this case are
the following as seen on the Img. 1-1, then as I was following the instructions I created both
the helper function to find a supported device for depth testing and the
createDepthResrouces declaration and implementation, this function finds the
depthFormat and create vKImages for the depth with following the format.

```cpp
// --- Depth Resources ---
VkImage depthImage;
VkDeviceMemory depthImageMemory;
VkImageView depthImageView;
VkFormat depthFormat;
```

```cpp
/* Depth Resources */
void createDepthResources();
```

```cpp
// --- Depth Buffer Implementations ---
void HelloTriangleApplication::createDepthResources() {
    depthFormat = findDepthFormat();
    
    // Re-use your existing createImage helper function
    createImage(swapChainExtent.width, swapChainExtent.height, depthFormat,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImage, depthImageMemory
    );

    // Re-use your existing createImageView helper function
    depthImageView = createImageView(depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
}
```

```cpp
// --- Helper Implementations ---
VkFormat HelloTriangleApplication::findSupportedFormat(const std::vector<VkFormat>& candidates, 
    VkImageTiling tiling, VkFormatFeatureFlags features) {
    for (VkFormat format : candidates) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);
        
        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
            return format;
        } else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
            return format;
        }
    }
    throw std::runtime_error("failed to find supported format!");
}
```

Then after that I had to make some changes to the graphics pipeline, I added the depth stencil state create info struct and assign it on the rendering create info struct’s depthAttachmentFormat. And also on the pipelineCreateInfo’s pDepthStencilState.

```cpp
// --- Pipeline Depth Stencil State Configuration ---
VkPipelineDepthStencilStateCreateInfo depthStencil{};
depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
depthStencil.depthTestEnable = VK_TRUE;
depthStencil.depthWriteEnable = VK_TRUE; // Enable writing
depthStencil.depthCompareOp = VK_COMPARE_OP_LESS; // Standard for perspective
depthStencil.depthBoundsTestEnable = VK_FALSE;
depthStencil.stencilTestEnable = VK_FALSE;
```

```cpp
VkPipelineRenderingCreateInfo renderingCreateInfo{};
renderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
renderingCreateInfo.colorAttachmentCount = 1;
renderingCreateInfo.pColorAttachmentFormats = &swapChainImageFormat;
renderingCreateInfo.depthAttachmentFormat = depthFormat;

VkGraphicsPipelineCreateInfo pipelineInfo{};
pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
pipelineInfo.pNext = &renderingCreateInfo;
pipelineInfo.stageCount = 2;
pipelineInfo.pStages = shaderStages;
pipelineInfo.pVertexInputState = &vertexInputInfo;
pipelineInfo.pInputAssemblyState = &inputAssembly;
pipelineInfo.pViewportState = &viewportState;
pipelineInfo.pRasterizationState = &rasterizer;
pipelineInfo.pMultisampleState = &multisampling;
pipelineInfo.pColorBlendState = &colorBlending;
pipelineInfo.pDynamicState = &dynamicState;
pipelineInfo.layout = pipelineLayout;
pipelineInfo.renderPass = VK_NULL_HANDLE; // Used with dynamic rendering
pipelineInfo.subpass = 0;
pipelineInfo.pDepthStencilState = &depthStencil;
```

Finally on the command buffer, I also had to add the depthAttachment struct to the pDepthAttachment value on the rendering Info struct command.

```cpp
VkRenderingAttachmentInfo depthAttachment{};
depthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
depthAttachment.imageView = depthImageView;
depthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; // Clear depth at start of frame
depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
depthAttachment.clearValue.depthStencil = { 1.0f, 0 };

VkRenderingInfo renderingInfo{};
renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
renderingInfo.renderArea = { {0, 0}, swapChainExtent };
renderingInfo.layerCount = 1;
renderingInfo.colorAttachmentCount = 1;
renderingInfo.pColorAttachments = &colorAttachment;
renderingInfo.pDepthAttachment = &depthAttachment;

vkCmdBeginRendering(commandBuffer, &renderingInfo);
vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
```

I got the cube to render properly with depth.

![Cube rendered with depth buffer](<../markdown-resources/Complete Real Time Graphics Lab Book (1 - 8)/img7-1.png> "Cube rendered with depth buffer")]]

- Test data
N/A

- Sample output
A render of a cube with depth.

- Reflection

    - *Reflect on what you have learnt from this exercise.*
      That, as always with Vulkan you have to add structs and follow the simple pipeline of:

        1. Find resources.
        2. Add them to where needed.
        3. Adjust graphics pipeline and command buffer.

    - *Did you make any mistakes?*
      No.

    - *In what way has your knowledge improved?*
      In how to implement the first steps towards depth buffering.

- Questions

    - *Is there anything you would like to ask?*
      No.

### Exercise 2: IMPLEMENTING THE SKYBOX

#### Question
1. Goal: Render a 6-sided cube map as a non-interactive background.
2. Implementation:
    1. **C++ (Assets)**: Load 6 separate images for the skybox faces (front, back, top, bottom, left, right).
    2. **C++ (Vulkan)**:
        1. In createImage() used in Lab 6, set imageType = VK_IMAGE_TYPE_2D, arrayLayers = 6, and flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT
        2. Create an array of VkImage from the 6 images. The order of the six images in the array should be as follows.
            | Syntax | Description | Direction |
            | 0 | +X | Right |
            | 1 | -X | Right |
            | 2 | +Y | Right |
            | 3 | -Y | Right |
            | 4 | +Z | Right |
            | 5 | -Z | Right |

        3. Create a VkImageView. Set viewType = VK_IMAGE_VIEW_TYPE_CUBE and layerCount = 6
        4. Create a VkSampler.
    3. **C++ (Pipeline)**:
        1. Create a new skyboxPipeline: VkPipeline skyboxPipeline = VK_NULL_HANDLE;    
        2. Write a createSkyboxPipeline() like creating the original createGraphicsPipeline() method. In the method, describe the VkPipelineDepthStencilStateCreateInfo in the following way by specifying depthTestEnable = VK_TRUE, depthWriteEnable = VK_FALSE. We want the skybox to be "at" the far plane, so we use VK_COMPARE_OP_LESS_OR_EQUAL 
            ```cpp
            VkPipelineDepthStencilCreateInfo depthStencil {};
            DepthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
            DepthStencil.depthTestEnable = VK_TRUE;
            DepthStencil.depthWriteEnable = VK_FALSE;
            DepthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
            ```      
        3. Configure rasterization state VkPipelineRasterizationStateCreateInfo:
            `cullMode = VK_CULL_MODE_FRONT_BIT`.
           Image we are inside the cube, so we must cull the front faces to see the back faces.              
4. Shaders (GLSL): Create the following shaders and implement them in the Vulkan API:    
        1. skybox.vert: The goal is to make the cube seem infinitely far and always centered on the camera (please read the lecture note for the detailed description of the principle).
        ```glsl
        layout(binding = 0) uniform UniformBufferObject {
        mat4 model;
        mat4 view;
        mat4 proj;
        vec3 eyePos;
        } ubo;
        layout(location = 0) in vec3 inPosition;
        // ... ...
        layout(location = 1) out vec3 viewDir;
        void main() {
            viewDir = inPosition;
            vec3 wPos=inPosition+ubo.eyePos;
            gl_Position = ubo.proj * ubo.view * vec4(inPosition, 1.0);
        }
        ```
        2. Skybox.frag:
        ```glsl
        layout(location = 0) in vec3 viewDir;
        layout(binding = 1) uniform samplerCube skySampler;
        layout(location = 0) out vec4 outColor;
        void main() {
            // Sample the cubemap using the vertex's position as a 3D
            direction
            outColor = texture(skySampler, fragTexCoord);
        }
        ```
    5. C++ (Rendering): In your render loop, draw the skybox first, before any solid objects. This allows its depth (1.0) to fill the depth buffer, so any objects in front of it will be drawn correctly.
3. Expected Outcome: Your scene is now surrounded by a 360-degree sky. When you rotate the camera, the skybox rotates with you, but it appears to be infinitely far away. The following image is the view at the eye position at (2, 0, 5).

![Example of skybox rendering](<../markdown-resources/Complete Real Time Graphics Lab Book (1 - 8)/img7-2-example.png> "Example of skybox rendering")]]

#### Solution
For this one I had to do a lot of little changes in order to integrate the skybox correctly. First I created the class members for the skybox images as well as the descriptor layouts, pipeline layout, its pipeline and descriptor sets.

```cpp
// --- Skybox Members ---
VkImage skyboxImage;
VkDeviceMemory skyboxImageMemory;
VkImageView skyboxImageView;
VkSampler skyboxSampler;

VkDescriptorSetLayout skyboxDescriptorSetLayout;
VkPipelineLayout skyboxPipelineLayout;
VkPipeline skyboxPipeline;
std::vector<VkDescriptorSet> skyboxDescriptorSets;
```

And as welI as the function declaration needed to render the skybox.

```cpp
/* Skybox Resources */
void createCubeMapResources();
void createSkyboxPipeline();
void createSkyboxDescriptorSets();
```

I had to call the functions on the initVulkan function.

```cpp
void HelloTriangleApplication::initVulkan() {
    createInstance();
    setupDebugMessenger();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
    createSwapChain();
    createImageViews();
    createDescriptorSetLayout();
    createGraphicsPipeline();

    // --- FIX: Move createCommandPool UP here ---
    createCommandPool();

    // Skybox setup
    createCubeMapResources();
    createSkyboxPipeline();

    // loadModel();

    // Texture mapping setup
    createTextureImage();
    createTextureImageView();
    createTextureSampler();
}
```

I had to implement the creation of resources of the cube map, I added the 6 files from the cube map and then I had t load the images, although first I needed the dimensions, after
that I looped through the images with the height and width. Then I had to create a staging buffer and copy the data to the buffer. Then finally via the struct of VkImageCreationInfo I created the image.

```cpp
// --- Skybox Implementations ---
void HelloTriangleApplication::createCubeMapResources() {
    // 1. Image Paths (Order: +X, -X, +Y, -Y, +Z, -Z)
    std::vector<std::string> fileNames = {
        "C:\\Users\\949145\\GitHub\\GraphicsLab7\\resources\\cubemap\\cubemap_0(+X).jpg",
        "C:\\Users\\949145\\GitHub\\GraphicsLab7\\resources\\cubemap\\cubemap_1(-X).jpg",
        "C:\\Users\\949145\\GitHub\\GraphicsLab7\\resources\\cubemap\\cubemap_2(+Y).jpg",
        "C:\\Users\\949145\\GitHub\\GraphicsLab7\\resources\\cubemap\\cubemap_3(-Y).jpg",
        "C:\\Users\\949145\\GitHub\\GraphicsLab7\\resources\\cubemap\\cubemap_4(+Z).jpg",
        "C:\\Users\\949145\\GitHub\\GraphicsLab7\\resources\\cubemap\\cubemap_5(-Z).jpg"
    };

    int texWidth, texHeight, texChannels;
    stbi_uc* pixels[6];

    // Load the first image to get dimensions
    pixels[0] = stbi_load(fileNames[0].c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    if (!pixels[0]) throw std::runtime_error("Failed to load skybox image: " + fileNames[0]);

    VkDeviceSize layerSize = texWidth * texHeight * 4;
    VkDeviceSize imageSize = layerSize * 6; // Total size for 6 faces

    // Load the other 5 images
    for (int i = 1; i < 6; i++) {
        pixels[i] = stbi_load(fileNames[i].c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
        if (!pixels[i]) throw std::runtime_error("Failed to load skybox image: " + fileNames[i]);
    }

    // 2. Create Staging Buffer
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
        stagingBuffer, stagingBufferMemory);

    // 3. Copy Data to Staging Buffer
    void* data;
    vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data);
    for (int i = 0; i < 6; i++) {
        // Offset the pointer for each layer
        memcpy(static_cast<char*>(data) + (layerSize * i), pixels[i], static_cast<size_t>(layerSize));
        stbi_image_free(pixels[i]);
    }
    vkUnmapMemory(device, stagingBufferMemory);

    // 4. Create Cube Map Image
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = texWidth;
    imageInfo.extent.height = texHeight;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 6;
    imageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT; // Critical for Skybox!

    if (vkCreateImage(device, &imageInfo, nullptr, &skyboxImage) != VK_SUCCESS) {
        throw std::runtime_error("failed to create skybox image!");
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device, skyboxImage, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    if (vkAllocateMemory(device, &allocInfo, nullptr, &skyboxImageMemory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate skybox image memory!");
    }

    vkBindImageMemory(device, skyboxImage, skyboxImageMemory, 0);
}
```

Continuing with the implementation of creating the resources for the skybox cubemap I get the images of TYPE cube, create the image view and then create the sampler.

```cpp
// 5. Transition and Copy
// Pass '6' as the last argument for layerCount
transitionImageLayout(skyboxImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, 6);

// Copy Buffer to Image (All 6 layers)
VkCommandBuffer commandBuffer = beginSingleTimeCommands();
std::vector<VkBufferImageCopy> bufferCopyRegions;
for (uint32_t i = 0; i < 6; i++) {
    VkBufferImageCopy region{};
    region.bufferOffset = layerSize * i;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = i;
    region.imageSubresource.layerCount = 1;
    region.imageExtent = { (uint32_t)texWidth, (uint32_t)texHeight, 1 };
    bufferCopyRegions.push_back(region);
}

vkCmdCopyBufferToImage(commandBuffer, stagingBuffer, skyboxImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, static_cast<uint32_t>(bufferCopyRegions.size()), bufferCopyRegions.data());
endSingleTimeCommands(commandBuffer);

// Pass '6' as the last argument here as well
transitionImageLayout(skyboxImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, 6);

vkDestroyBuffer(device, stagingBuffer, nullptr);
vkFreeMemory(device, stagingBufferMemory, nullptr);

// 6. Create Image View (Type = CUBE)
VkImageViewCreateInfo viewInfo{};
viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
viewInfo.image = skyboxImage;
viewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE; // Important!
viewInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
viewInfo.subresourceRange.baseMipLevel = 0;
viewInfo.subresourceRange.levelCount = 1;
viewInfo.subresourceRange.baseArrayLayer = 0;
viewInfo.subresourceRange.layerCount = 6;

if (vkCreateImageView(device, &viewInfo, nullptr, &skyboxImageView) != VK_SUCCESS) {
    throw std::runtime_error("failed to create skybox image view!");
}

// 7. Create Sampler
VkSamplerCreateInfo samplerInfo{};
samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
samplerInfo.magFilter = VK_FILTER_LINEAR;
samplerInfo.minFilter = VK_FILTER_LINEAR;
samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
samplerInfo.anisotropyEnable = VK_TRUE;
samplerInfo.maxAnisotropy = 16.0f;
samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
samplerInfo.unnormalizedCoordinates = VK_FALSE;
samplerInfo.compareEnable = VK_FALSE;
samplerInfo.compareOp = VK_COMPARE_OP_NEVER;
samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

if (vkCreateSampler(device, &samplerInfo, nullptr, &skyboxSampler) != VK_SUCCESS) {
    throw std::runtime_error("failed to create skybox sampler!");
}
```

Then I had to create the graphics pipeline for the skybox, by creating the descriptors layout,
creating the shaders, compiling them in to getto get their .spv compiled versions via the
glslc included in the Vulkan SDK. And follow the pipeline as needed and adding the depth
stencil.

```cpp
void HelloTriangleApplication::createSkyboxPipeline() {
    // 1. Descriptor Set Layout (Binding 0: UBO, Binding 1: SamplerCube)
    VkDescriptorSetLayoutBinding uboLayoutBinding{};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutBinding samplerLayoutBinding{};
    samplerLayoutBinding.binding = 1;
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    std::array<VkDescriptorSetLayoutBinding, 2> bindings = { uboLayoutBinding, samplerLayoutBinding };
    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &skyboxDescriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create skybox descriptor set layout!");
    }

    // 2. Pipeline Layout
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &skyboxDescriptorSetLayout;

    if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &skyboxPipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create skybox pipeline layout!");
    }

    // 3. Shaders
    auto vertShaderCode = readFile("shaders/skybox.vert.spv"); // Make sure to compile skybox.vert!
    auto fragShaderCode = readFile("shaders/skybox.frag.spv"); // Make sure to compile skybox.frag!

    VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
    VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

    // 4. Vertex Input (Skybox only needs Position)
    // We can reuse GeometryVertex but we only use location 0
    auto bindingDescription = Vertex::getBindingDescription();
    auto attributeDescriptions = Vertex::getAttributeDescriptions(); // We pass all, but shader only uses loc 0

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    // 5. Viewport & Scissor (Dynamic)
    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    // 6. Rasterizer (CULL FRONT for Skybox)
    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_FRONT_BIT; // We are inside the cube
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    // 7. Depth Stencil (LESS_OR_EQUAL, Write False)
    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_FALSE; // Skybox is background
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL; // Far plane is 1.0
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    std::vector<VkDynamicState> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    // Rendering Info
    VkPipelineRenderingCreateInfo renderingCreateInfo{};
    renderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    renderingCreateInfo.colorAttachmentCount = 1;
    renderingCreateInfo.pColorAttachmentFormats = &swapChainImageFormat;
    renderingCreateInfo.depthAttachmentFormat = depthFormat;

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.pNext = &renderingCreateInfo;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = skyboxPipelineLayout;
    pipelineInfo.renderPass = VK_NULL_HANDLE;
    pipelineInfo.pDepthStencilState = &depthStencil; // Add depth stencil!

    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &skyboxPipeline) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create skybox graphics pipeline!");
    }

    vkDestroyShaderModule(device, fragShaderModule, nullptr);
    vkDestroyShaderModule(device, vertShaderModule, nullptr);
}

```

Because the graphics pipeline needs the descriptor sets I created the function to create those Skybox Descriptor Sets.

```cpp
void HelloTriangleApplication::createSkyboxDescriptorSets() {
    std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, skyboxDescriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    allocInfo.pSetLayouts = layouts.data();

    skyboxDescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
    if (vkAllocateDescriptorSets(device, &allocInfo, skyboxDescriptorSets.data()) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate skybox descriptor sets!");
    }

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        // 1. UBO Info (Reuse the same UBO as the main scene!)
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = uniformBuffers[i];
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(UniformBufferObject);

        // 2. Skybox Texture Info
        VkDescriptorImageInfo skyboxInfo{};
        skyboxInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        skyboxInfo.imageView = skyboxImageView;
        skyboxInfo.sampler = skyboxSampler;

        std::array<VkWriteDescriptorSet, 2> descriptorWrites{};

        // Binding 0: Vertex Shader UBO
        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = skyboxDescriptorSets[i];
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo = &bufferInfo;

        // Binding 1: Fragment Shader SamplerCube
        descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstSet = skyboxDescriptorSets[i];
        descriptorWrites[1].dstBinding = 1;
        descriptorWrites[1].dstArrayElement = 0;
        descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].pImageInfo = &skyboxInfo;

        vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
    }
}
```

Evidence of the creation of the skybox shaders.

![Skybox shader](<../markdown-resources/Complete Real Time Graphics Lab Book (1 - 8)/img7-2-shaders.png> "Skybox shader")]

Implementation of shaders: vertex and fragment for the skybox.

```glsl
#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec3 eyePos;
} ubo;

layout(location = 0) in vec3 inPosition;

layout(location = 1) out vec3 viewDir;

void main() {
    // viewDir is the direction vector for cubemap sampling
    viewDir = inPosition;

    // Use only the rotational component of the view matrix (mat3)
    // to remove camera translation, keeping the skybox centered.
    mat4 rotView = mat4(mat3(ubo.view));

    // Calculate clip position
    vec4 clipPos = ubo.proj * rotView * vec4(inPosition, 1.0);

    // The trick: Set gl_Position.w = gl_Position.z to force z/w = 1.0 after perspective divide.
    // This makes the skybox appear at the maximum depth (far plane).
    gl_Position = clipPos.xyww;
}
```

```glsl
#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 1) in vc3 viewDir; // Must match location from vert shader

layout(binding = 1) uniform samplerCube skySampler;

layout(location = 0) out vec4 outColor;

void main() {
    // Sample the cubemap using the certex's poisition as a 3D direction
    outColor = texture(skySampler, viewDir);
}_
```

Then I had to change the transition Image Layout for accepting different layers.

```cpp
// Update signature here too
void HelloTriangleApplication::transitionImageLayout(VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout, VkImageAspectFlags aspectMask, uint32_t layerCount) {
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;

    barrier.subresourceRange.aspectMask = aspectMask;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;

    // --- CHANGE IS HERE ---
    barrier.subresourceRange.layerCount = layerCount;
    // ----------------------

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    }
    else {
        throw std::invalid_argument("unsupported layout transition!");
    }

    vkCmdPipelineBarrier(
        commandBuffer,
        sourceStage, destinationStage,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier
    );

    endSingleTimeCommands(commandBuffer);
}
```

I changed the Combined image sampler to add one more for the skybox.

```cpp
void HelloTriangleApplication::createDescriptorPool() {
    std::array<VkDescriptorPoolSize, 2> poolSizes{};

    // 1. Uniform Buffers: We need 1 for the main object + 1 for the skybox (per frame)
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) * 2;

    // 2. Image Samplers: 3 for main object (Color, Normal, Height) + 1 for Skybox (per frame)
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) * 4;

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();

    // Max sets = Main Sets + Skybox Sets
    poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) * 2;

    if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create descriptor pool!");
    }
}
```

I had to apply changes to the record command buffer to accept the section of the skybox

```cpp
// Start Rendering
vkCmdBeginRendering(commandBuffer, &renderingInfo);

// --- 1. DRAW SKYBOX FIRST ---
// We assume meshBuffers[0] is the CUBE we created in initVulkan
vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, skyboxPipeline);

// Bind Skybox Descriptors
vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, skyboxPipelineLayout, 0, 1, &skyboxDescriptorSets[currentFrame], 0, nullptr);

// Bind Cube Vertex/Index Buffers
VkBuffer vertexBuffers[] = { meshBuffers[0].vertexBuffer };
VkDeviceSize offsets[] = { 0 };
vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
vkCmdBindIndexBuffer(commandBuffer, meshBuffers[0].indexBuffer, 0, VK_INDEX_TYPE_UINT32);

// Draw the Skybox Cube
vkCmdDrawIndexed(commandBuffer, meshBuffers[0].indexCount, 1, 0, 0, 0);

// --- 2. DRAW SCENE OBJECTS ---
vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

VkViewport viewport{};
viewport.width = static_cast<float>(swapChainExtent.width);
viewport.height = static_cast<float>(swapChainExtent.height);
viewport.maxDepth = 1.0f;
vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

VkRect2D scissor{};
scissor.extent = swapChainExtent;
vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[currentFrame], 0, nullptr);

// Draw all meshes (including the cube again, but with the normal shader)
// Note: If you only want the skybox in the background and not a solid cube in the center,
// you might want to comment out drawing meshBuffers[0] here, or move meshBuffers[0] to a specific location.
for (size_t i = 0; i < meshBuffers.size(); ++i) {
    VkBuffer vBuffers[] = { meshBuffers[i].vertexBuffer };
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vBuffers, offsets);
    vkCmdBindIndexBuffer(commandBuffer, meshBuffers[i].indexBuffer, 0, VK_INDEX_TYPE_UINT32);

    MaterialPushConstant material{};
    material.ambient = glm::vec3(0.5f, 0.5f, 0.5f);
    material.diffuse = glm::vec3(1.0f, 1.0f, 1.0f);
    material.specular = glm::vec3(1.0f, 1.0f, 1.0f);
    material.shininess = 32.0f;

    vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(MaterialPushConstant), &material);

    vkCmdDrawIndexed(commandBuffer, meshBuffers[i].indexCount, 1, 0, 0, 0);
}

vkCmdEndRendering(commandBuffer);

VkImageMemoryBarrier2 imageBarrierToPresent{};
imageBarrierToPresent.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
imageBarrierToPresent.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
imageBarrierToPresent.srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
imageBarrierToPresent.dstStageMask = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT;
imageBarrierToPresent.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
imageBarrierToPresent.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
imageBarrierToPresent.image = swapChainImages[imageIndex];
imageBarrierToPresent.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

VkDependencyInfo dependencyInfoToPresent{};
dependencyInfoToPresent.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
dependencyInfoToPresent.imageMemoryBarrierCount = 1;
dependencyInfoToPresent.pImageMemoryBarriers = &imageBarrierToPresent;
vkCmdPipelineBarrier2(commandBuffer, &dependencyInfoToPresent);

if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
    throw std::runtime_error("Failed to record command buffer!");
}
```

While I didn’t delete the rotating cube I left it there in order to test the depth and the skybox.

![Skybox rendered with rotating cube](<../markdown-resources/Complete Real Time Graphics Lab Book (1 - 8)/img7-2.png> "Skybox rendered with rotating cube")]]

- Test data
N/A

- Sample output
A render of a skybox with the previous cube spinning. (I wanted to test the depth).

- Reflection

    - *Reflect on what you have learnt from this exercise.*
      All the process to create a pipeline with new shaders. Especially the compilation of the shaders that’s something new.

    - *Did you make any mistakes?*
      No.

    - *In what way has your knowledge improved?*
      In the compilation of shaders and remember the process to get rendering images.

- Questions

    - *Is there anything you would like to ask?*
      No.

### Exercise 3: IMPLEMENTING REFLECTIONS

#### Question
1. Goal: Make one of your solid objects (e.g., a sphere or cube) perfectly reflective, like a mirror
2. Implementation:
    1. C++: Ensure your main object's descriptor set also includes the skySampler from Exercise 2 (at a different binding).
    2. Shaders (GLSL): Modify your main solid object's fragment shader.
        1. Add layout(binding = 1) uniform samplerCube skySampler;.
        2. Instead of calculating lighting, calculate the reflection vector.
            ```glsl
             // ... in variables ...
             layout(location = 1) in vec3 inWorldPos;
             layout(location = 2) in vec3 inWorldNormal;
             // ... UBO ...
             vec3 viewPos;
             void main() {
                vec3 N = normalize(inWorldNormal);
                vec3 V = normalize(ubo.viewPos - inWorldPos); // View vector
                // Calculate the reflection vector
                vec3 R = reflect(-V, N); // reflect() expects incident vector
                // Sample the skybox with the reflection vector
                vec3 reflectionColor = texture(skySampler, R).rgb;
                outColor = vec4(reflectionColor, 1.0);
             }
             ```
        3. C++ (Rendering): Draw this object normally with your pipelineSolid.
3. Expected Outcome:The object will act as a perfect mirror, reflecting the skybox. As you move the camera, the reflection will change realistically.

![Example of reflection rendering](<../markdown-resources/Complete Real Time Graphics Lab Book (1 - 8)/img7-3-example.png> "Example of reflection rendering")]]

#### Solution
For this exercise I had to change the createDescriptorLayout to add a new binding and the descriptorSetPool and update the descriptor count to 5

```cpp
void HelloTriangleApplication::createDescriptorSetLayout() {
    VkDescriptorSetLayoutBinding uboLayoutBinding{};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutBinding colorSamplerLayoutBinding{};
    colorSamplerLayoutBinding.binding = 1;
    colorSamplerLayoutBinding.descriptorCount = 1;
    colorSamplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    colorSamplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutBinding normalSamplerLayoutBinding{};
    normalSamplerLayoutBinding.binding = 2;
    normalSamplerLayoutBinding.descriptorCount = 1;
    normalSamplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    normalSamplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutBinding heightSamplerLayoutBinding{};
    heightSamplerLayoutBinding.binding = 3;
    heightSamplerLayoutBinding.descriptorCount = 1;
    heightSamplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    heightSamplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    // --- ADD THIS NEW BINDING ---
    VkDescriptorSetLayoutBinding skyboxSamplerLayoutBinding{};
    skyboxSamplerLayoutBinding.binding = 4; // New Binding ID
    skyboxSamplerLayoutBinding.descriptorCount = 1;
    skyboxSamplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    skyboxSamplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    // Update array size to 5 and add the new binding
    std::array<VkDescriptorSetLayoutBinding, 5> bindings = {
        uboLayoutBinding,
        colorSamplerLayoutBinding,
        normalSamplerLayoutBinding,
        heightSamplerLayoutBinding,
        skyboxSamplerLayoutBinding // Add here
    };

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create descriptor set layout!");
    }
}
```

```cpp
void HelloTriangleApplication::createDescriptorPool() {
    std::array<VkDescriptorPoolSize, 2> poolSizes{};

    // 1. Uniform Buffers: We need 1 for the main object + 1 for the skybox (per frame)
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) * 2;

    // 2. Image Samplers: 3 for main object (Color, Normal, Height) + 1 for Skybox (per frame)
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    // Change the multiplier from * 4 to * 5
    poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) * 5;

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();

    // Max sets = Main Sets + Skybox Sets
    poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) * 2;

    if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create descriptor pool!");
    }
}
```

On the createDescriptorSets I had to add the dkybox reflection binding as the fifth one.

```cpp
// --- CREATE SKYBOX INFO ---
VkDescriptorImageInfo skyboxInfo{};
skyboxInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
skyboxInfo.imageView = skyboxImageView; // Use the Skybox View
skyboxInfo.sampler = skyboxSampler;     // Use the Skybox Sampler

std::array<VkWriteDescriptorSet, 5> descriptorWrites{}; // Increase array size to 5

// (1) Uniform Buffer
descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
descriptorWrites[0].dstSet = descriptorSets[i];
descriptorWrites[0].dstBinding = 0;
descriptorWrites[0].dstArrayElement = 0;
descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
descriptorWrites[0].descriptorCount = 1;
descriptorWrites[0].pBufferInfo = &bufferInfo;

// (2) Color Texture Sampler
descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
descriptorWrites[1].dstSet = descriptorSets[i];
descriptorWrites[1].dstBinding = 1;
descriptorWrites[1].dstArrayElement = 0;
descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
descriptorWrites[1].descriptorCount = 1;
descriptorWrites[1].pImageInfo = &colorImageInfo;

// (3) Normal Map Sampler
descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
descriptorWrites[2].dstSet = descriptorSets[i];
descriptorWrites[2].dstBinding = 2;
descriptorWrites[2].dstArrayElement = 0;
descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
descriptorWrites[2].descriptorCount = 1;
descriptorWrites[2].pImageInfo = &normalImageInfo;

// (4) Height Map Sampler
descriptorWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
descriptorWrites[3].dstSet = descriptorSets[i];
descriptorWrites[3].dstBinding = 3;
descriptorWrites[3].dstArrayElement = 0;
descriptorWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
descriptorWrites[3].descriptorCount = 1;
descriptorWrites[3].pImageInfo = &heightImageInfo;

// (5) Skybox Reflection Sampler (Binding 4)
descriptorWrites[4].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
descriptorWrites[4].dstSet = descriptorSets[i];
descriptorWrites[4].dstBinding = 4; // Binding 4
descriptorWrites[4].dstArrayElement = 0;
descriptorWrites[4].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
descriptorWrites[4].descriptorCount = 1;
descriptorWrites[4].pImageInfo = &skyboxInfo;

vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
```

Then I had to change the cube’s shaders in order to have reflection. I even add the bump but you can also render it without it to see the reflection better.

```glsl
#version 450

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 model; mat4 view; mat4 proj;
    vec3 lightPos1; vec3 lightColor1; vec3 lightPos2; vec3 lightColor2;
    vec3 eyePos;
} ubo;

layout(push_constant) uniform MaterialPushConstant {
    vec3 ambient; vec3 diffuse; vec3 specular; float shininess;
} material;

// Texture samplers
layout(set = 0, binding = 1) uniform sampler2D colSampler;
layout(set = 0, binding = 2) uniform sampler2D normalSampler;
layout(set = 0, binding = 3) uniform sampler2D heightSampler;
layout(set = 0, binding = 4) uniform samplerCube skySampler;

// Inputs from vertex shader
layout(location = 0) in vec3 fragColor;
layout(location = 3) in vec2 fragTexCoord;
layout(location = 7) in vec3 fragT;
layout(location = 8) in vec3 fragB;
layout(location = 9) in vec3 fragN;
layout(location = 10) in vec3 fragWorldPos;
layout(location = 11) in vec3 viewDir_tangent;

layout(location = 0) out vec4 outColor;

void main() {
    // --- 1. NORMAL CALCULATION ---
    
    // Calculate TBN Matrix
    mat3 TBN = mat3(fragT, fragB, fragN);
    vec3 N_world;

    // [OPTION A] Use Bump/Normal Mapping (Detailed surface)
    vec3 normalSample = texture(normalSampler, fragTexCoord).rgb;
    vec3 N_tangent = normalize(normalSample * 2.0 - 1.0);
    N_world = normalize(TBN * N_tangent);

    // [OPTION B] Use Flat Geometry Normal (Smooth surface)
    // Uncomment the line below and comment out Option A to see a flat reflection
    // N_world = normalize(fragN);

    // --- 2. REFLECTION CALCULATION ---

    // Calculate View Vector (Camera to Fragment)
    // vec3 viewDir = normalize(fragWorldPos - ubo.eyePos.xyz);

    // Calculate Reflection Vector
    // Input vector must point TO surface, so -viewDir doesn't work if viewDir is Frag->Eye
    // Since we calculated viewDir as (Frag - Eye), it points FROM Eye TO Frag.
    // reflect() expects incident vector (Light->Surface). So we use -viewDir if viewDir is Frag->Eye.
    
    vec3 V = normalize(ubo.eyePos - fragWorldPos); // Vector points TO Camera
    vec3 R = reflect(-V, N_world); // -V points TO surface

    // Sample the Skybox
    vec3 reflectionColor = texture(skySampler, R).rgb;

    // --- 3. FINAL OUTPUT ---

    // Get base texture color
    vec3 albedo = texture(colSampler, fragTexCoord).rgb;

    // [OPTION 1] Realistic Mix (50% Texture, 50% Reflection)
    outColor = vec4(mix(albedo, reflectionColor, 0.5), 1.0);

    // [OPTION 2] Pure Mirror (To test reflection clearly)
    // outColor = vec4(reflectionColor, 1.0);
}
```

At the end I got the cube to render the reflection.

![Reflection rendered on cube with bump mapping](<../markdown-resources/Complete Real Time Graphics Lab Book (1 - 8)/img7-3.png> "Reflection rendered on cube with bump mapping")]

- Test data
N/A

- Sample output
A render of a Quad with height map bumping.

- Reflection

    - *Reflect on what you have learnt from this exercise.*
      I learnt about implementing the reflect visuals on to an object according to the skybox images.

    - *Did you make any mistakes?*
      No.

    - *In what way has your knowledge improved?*
      In how to implement and add the changes needed to have the reflection being rendered.

- Questions

    - *Is there anything you would like to ask?*
      No.

### Exercise 4: IMPLEMENTING REFRACTION

#### Question
1. Goal: Simulate a transparent, refractive object (like glass or water).
2. Implementation:
    1. Shaders (GLSL): Modify the fragment shader from Exercise 3:
        ```glsl
        Instead of reflect, use refract.
        // ... same as Exercise 3 ...
        void main() {
            vec3 N = normalize(inWorldNormal);
            vec3 V = normalize(ubo.viewPos - inWorldPos);
            float IOR = 1.00 / 1.33; // Index of Refraction (air to water)
            // Calculate the refraction vector
            vec3 R = refract(-V, N, IOR);
            vec3 refractionColor = texture(skySampler, R).rgb;
            outColor = vec4(refractionColor, 1.0);
        }
        ```
3. Expected Outcome: The object now appears transparent, and the skybox seen through it is distorted, as if looking through a lens or a block of glass

![Example of refraction rendering](<../markdown-resources/Complete Real Time Graphics Lab Book (1 - 8)/img7-4-example.png> "Example of refraction rendering")]]

#### Solution
For this one I just had to follow the instructions to have the refraction going. Changing the algorithm to get the refraction going on the cube.

```glsl
#version 450

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec3 lightPos1;
    vec3 lightColor1;
    vec3 lightPos2;
    vec3 lightColor2;
    vec3 eyePos;
} ubo;

layout(push_constant) uniform MaterialPushConstant {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float shininess;
} material;

// Texture samplers
layout(set = 0, binding = 1) uniform sampler2D colSampler;
layout(set = 0, binding = 2) uniform sampler2D normalSampler;
layout(set = 0, binding = 3) uniform sampler2D heightSampler;
// --- NEW SKYBOX SAMPLER ---
layout(set = 0, binding = 4) uniform samplerCube skySampler;

// Inputs
layout(location = 0) in vec3 fragColor;
layout(location = 3) in vec2 fragTexCoord;
layout(location = 7) in vec3 fragT;
layout(location = 8) in vec3 fragB;
layout(location = 9) in vec3 fragN;
layout(location = 10) in vec3 fragWorldPos;
layout(location = 11) in vec3 viewDir_tangent;

layout(location = 0) out vec4 outColor;

void main() {
    // --- 1. NORMAL CALCULATION ---
    mat3 TBN = mat3(fragT, fragB, fragN);
    vec3 N_world;

    // [OPTION A] Use Bump/Normal Mapping (Detailed surface)
    vec3 normalSample = texture(normalSampler, fragTexCoord).rgb;
    vec3 N_tangent = normalize(normalSample * 2.0 - 1.0);
    N_world = normalize(TBN * N_tangent);

    // [OPTION B] Use Flat Geometry Normal (Smooth surface)
    // N_world = normalize(fragN);

    // --- 2. VIEW VECTOR ---
    // Vector from Fragment to Camera (Eye)
    vec3 V = normalize(ubo.eyePos - fragWorldPos);

    // --- 3. CHOOSE EFFECT ---

    // [EFFECT 1] REFLECTION (Mirror)
    // -------------------------------------------------------
    // Incident vector I = -V (Camera to Fragment)
    vec3 R_reflect = reflect(-V, N_world);
    // vec3 color = texture(skySampler, R_reflect).rgb;
    // -------------------------------------------------------

    // [EFFECT 2] REFRACTION (Glass/Water)
    // -------------------------------------------------------
    // Ratio of indices of refraction (Air / Glass)
    float IOR = 1.00 / 1.33;

    // Calculate refraction vector
    // refract(Incident, Normal, eta)
    vec3 R_refract = refract(-V, N_world, IOR);

    // Sample the skybox using the refracted vector
    vec3 color = texture(skySampler, R_refract).rgb;
    // -------------------------------------------------------

    // --- 4. FINAL OUTPUT ---

    // Pure Refraction (Glass look)
    outColor = vec4(color, 1.0);

    // Optional: Mix with base color if you want "tinted" glass
    // vec3 albedo = texture(colSampler, fragTexCoord).rgb;
    // outColor = vec4(mix(albedo, color, 0.3), 1.0);
}
```

![Refraction rendered on cube](<../markdown-resources/Complete Real Time Graphics Lab Book (1 - 8)/img7-4.png> "Refraction rendered on cube")]

- Test data
N/A

- Sample output
A render of cube with refraction.

- Reflection

    - *Reflect on what you have learnt from this exercise.*
      The fact that now I know the differences of reflection and refraction and how the maths involved also changs depending in what you want to create..

    - *Did you make any mistakes?*
      No.

    - *In what way has your knowledge improved?*
      In that I can even combined with the albedo to get that glass factor to meshes.

- Questions

    - *Is there anything you would like to ask?*
      No.

### Exercise 5: SPRITE-BASED PARTICLE SYSTEM

#### Question
1. Goal: Create a simple fire or smoke effect using billboards and blending.
2. Implementation:
    1. **C++ (Data)**: Create a sequence of quads along the z-axis with identical size of [-1, 1]x[-1, 1] for xy-dimensions and an increased z-coordinate values varying from, say, 0 to 1. The value of z- coordinate can be used to identify the position of each particle in the vertex shader.
    2. **C++ (Time)**: Pass time to vertex shader as a uniform variable.
    3. **C++ (Pipeline)**: Create a particlePipeline and configure depth testing and colour blending.      
        1. Fragment Shader:VkPipelineDepthStencilStateCreateInfo:
              `depthTestEnable = VK_TRUE, depthWriteEnable = VK_FALSE`.
        2. 
        ```cpp
        VkPipelineColorBlendAttachmentState: blendEnable = VK_TRUE,
        srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
        dstColorBlendFactor = VK_BLEND_FACTOR_ON (for additive fire) or VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA (for smokey transparency).
        ```
        3. `VkPipelineRasterizationStateCreateInfo: cullMode = VK_CULL_MODE_NONE_BIT`
        4. Take the viewDir_tangent you calculated.
    4. **Shaders (GLSL)**:
        1. particle.vert: Implement billboarding. The vertex inPosition will be the particle center, and inTexCoord can store the quad's corner offset
           (e.g., (-1, -1), (1, 1)).
           ```glsl
            layout(location = 0) in vec3 inParticlePos;
            ... ... ...
            layout(location = 0) out vec2 texCoord;
            layout(location = 1) out t;
            ... ... ... ...
            // ... out ...
            #define particleSpeed 0.48
            #define particleSpread 20.48
            #define particleShape 0.37
            #define particleSize 6.0
            #define particleSystemHeight 60.0
            void main() {
                / slice the time and loop particles
                float t = fract(inParticlePos.z + particleSpeed * time);
                //reposition the quads based on their z-coordinate:
                vec3 pos;
                // Spread particles in a semi-random fashion
                pos.x = particleSpread * t * cos(50.0 * inParticlePos.z);
                pos.z = particleSpread * t * sin(120.0 * inParticlePos.z);
                // Find the inverse of the view matrix
                mat4 viewInv = inverse(view);
                //align quad orientation with view image plane (billboarding)
                vec3 BBPos =(pos.x * viewInv[0] + pos.y * viewInv[1]).xyz;
                //resize the quad
                pos += particleSize * BBPos;
                gl_Position = ubo.proj * ubo.view * vec4(worldPos, 1.0);
                texCoord = inParticlePos.xy;
                ... ... ...
            }
            ```
        2. particle.frag: Define the pixel colour at each location based on texCoord and fade the colour with time.
            ```glsl
            ... ... ...
            layout(location = 0) in vec2 texCoord;
            layout(location = 1) in t;
            layout(location = 0) out vec4 outColor;
            ... ... ...
            void main() {
                outColor = ... ... ... ;
            }
            ```
3. Expected Outcome: You will see a cloud of 2D sprites (e.g., 50-100) that look 3D, move according to your physics, and blend correctly with each other and the solid scene.

![Example of particle system rendering](<../markdown-resources/Complete Real Time Graphics Lab Book (1 - 8)/img7-5-example.png> "Example of particle system rendering")]]

#### Solution
For this exercise, just as before I have to do some changes and add functions and class
members to get the particles rendering. First I created the class members that I was going
to use for the particles and then I also created the declarations of the functions that I was
going to use.

```cpp
// --- Particle System Members ---
VkPipeline particlePipeline;
VkPipelineLayout particlePipelineLayout;
VkBuffer particleVertexBuffer;
VkDeviceMemory particleVertexBufferMemory;
VkBufer particleIndexBuffer;
VkDeviceMemory particleIndexBufferMemory;
uint32_t particleIndexCount;_
```

```cpp
/* Particle System Resources */
void createParticleResources();
void createParticlePipeline();
```

Just as before, I added the functions to init Vulkan and started to implement them later on in the code.

```cpp
void HelloTriangleApplication::initVulkan() {
    createInstance();
    setupDebugMessenger();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
    createSwapChain();
    createImageViews();
    createDescriptorSetLayout();
    createGraphicsPipeline();

    // --- FIX: Move createCommandPool UP here ---
    createCommandPool();

    // Skybox setup
    createCubeMapResources();
    createSkyboxPipeline();

    // Particle system setup
    createParticleResources();
    createParticlePipeline();
}
```

First, I had to create the resources needed to create the VkImageViews for the particle system rendering

```cpp
// --- Particle System Implementations ---
void HelloTriangleApplication::createParticleResources() {
    // We will create 100 particles
    const int particleCount = 100;
    std::vector<float> vertices; // Using raw floats for custom layout
    std::vector<uint32_t> indices;

    for (int i = 0; i < particleCount; i++) {
        // Create a unique Z "seed" for this particle
        float z = static_cast<float>(i) / static_cast<float>(particleCount);

        // 4 vertices per particle (Quad)
        // Format: x, y (corner), z (seed)
        vertices.push_back(-1.0f); vertices.push_back(-1.0f); vertices.push_back(z); // Bottom-Left
        vertices.push_back(1.0f);  vertices.push_back(-1.0f); vertices.push_back(z); // Bottom-Right
        vertices.push_back(1.0f);  vertices.push_back(1.0f);  vertices.push_back(z); // Top-Right
        vertices.push_back(-1.0f); vertices.push_back(1.0f);  vertices.push_back(z); // Top-Left

        // 6 indices (2 triangles)
        uint32_t baseIndex = i * 4;
        indices.push_back(baseIndex + 0);
        indices.push_back(baseIndex + 1);
        indices.push_back(baseIndex + 2);
        indices.push_back(baseIndex + 2);
        indices.push_back(baseIndex + 3);
        indices.push_back(baseIndex + 0);
    }

    particleIndexCount = static_cast<uint32_t>(indices.size());

    // Create Vertex Buffer
    VkDeviceSize vSize = sizeof(float) * vertices.size();
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(vSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
        stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(device, stagingBufferMemory, 0, vSize, 0, &data);
    memcpy(data, vertices.data(), (size_t)vSize);
    vkUnmapMemory(device, stagingBufferMemory);

    createBuffer(vSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, 
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, particleVertexBuffer, particleVertexBufferMemory);
    copyBuffer(stagingBuffer, particleVertexBuffer, vSize);

    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);

    // Create Index Buffer
    VkDeviceSize iSize = sizeof(uint32_t) * indices.size();
    createBuffer(iSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
        stagingBuffer, stagingBufferMemory);

    vkMapMemory(device, stagingBufferMemory, 0, iSize, 0, &data);
    memcpy(data, indices.data(), (size_t)iSize);
    vkUnmapMemory(device, stagingBufferMemory);

    createBuffer(iSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, 
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, particleIndexBuffer, particleIndexBufferMemory);
    copyBuffer(stagingBuffer, particleIndexBuffer, iSize);

    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
}
```

Then I had to create the graphics pipeline for the particle system, following the same pipeline as before with its due changes for particles.

```cpp
void HelloTriangleApplication::createParticlePipeline() {
    // 1. Push Constant Range (For Time)
    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT; // Used in Vert Shader
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(float); // Just one Float (Time)

    // 2. Pipeline Layout (Re-use standard descriptor set layout for UBO)
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout; // Use main layout for UBO
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

    if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &particlePipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create particle pipeline layout!");
    }

    // 3. Shaders
    auto vertShaderCode = readFile("shaders/particle.vert.spv");
    auto fragShaderCode = readFile("shaders/particle.frag.spv");
    VkShaderModule vertModule = createShaderModule(vertShaderCode);
    VkShaderModule fragModule = createShaderModule(fragShaderCode);

    VkPipelineShaderStageCreateInfo shaderStages[] = {
        {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0, VK_SHADER_STAGE_VERTEX_BIT, vertModule, "main", nullptr},
        {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0, VK_SHADER_STAGE_FRAGMENT_BIT, fragModule, "main", nullptr}
    };

    // 4. Vertex Input (Custom for Particles: vec3 only)
    VkVertexInputBindingDescription bindingDescription{};
    bindingDescription.binding = 0;
    bindingDescription.stride = 3 * sizeof(float); // x, y, z
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription attributeDescription{};
    attributeDescription.binding = 0;
    attributeDescription.location = 0;
    attributeDescription.format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescription.offset = 0;

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = 1;
    vertexInputInfo.pVertexAttributeDescriptions = &attributeDescription;

    // 5. Input Assembly & Viewport
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{ VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkPipelineViewportStateCreateInfo viewportState{ VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterizer{ VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_NONE; // Important: Don't cull particles!
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

    VkPipelineMultisampleStateCreateInfo multisampling{ VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    // 6. Depth Stencil (Test YES, Write NO)
    VkPipelineDepthStencilStateCreateInfo depthStencil{ VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_FALSE; // Transparent objects don't write depth
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;

    // 7. Blending (Additive for Fire)
    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_TRUE;

    // SrcAlpha * One (Additive) creates glow
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo colorBlending{ VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    std::vector<VkDynamicState> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    VkPipelineDynamicStateCreateInfo dynamicState{ VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
    dynamicState.dynamicStateCount = 2;
    dynamicState.pDynamicStates = dynamicStates.data();

    VkPipelineRenderingCreateInfo renderingCreateInfo{ VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO };
    renderingCreateInfo.colorAttachmentCount = 1;
    renderingCreateInfo.pColorAttachmentFormats = &swapChainImageFormat;
    renderingCreateInfo.depthAttachmentFormat = depthFormat;

    VkGraphicsPipelineCreateInfo pipelineInfo{ VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
    pipelineInfo.pNext = &renderingCreateInfo;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = particlePipelineLayout;

    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &particlePipeline) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create particle pipeline!");
    }

    vkDestroyShaderModule(device, vertModule, nullptr);
    vkDestroyShaderModule(device, fragModule, nullptr);
}
```

Then I had to change the record command buffer to get the particles rendering.

```cpp
// --- 3. DRAW PARTICLES ---
vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, particlePipeline);

// Bind Vertex/Index Buffers
VkBuffer pVertBuffers[] = { particleVertexBuffer };
VkDeviceSize pOffsets[] = { 0 };
vkCmdBindVertexBuffers(commandBuffer, 0, 1, pVertBuffers, pOffsets);
vkCmdBindIndexBuffer(commandBuffer, particleIndexBuffer, 0, VK_INDEX_TYPE_UINT32);

// Bind UBO (We reuse the same descriptor set for View/Proj matrices!)
vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, particlePipelineLayout, 0, 1, &descriptorSets[currentFrame], 0, nullptr);

// Push Time Constant
static auto startTime = std::chrono::high_resolution_clock::now();
auto currentTime = std::chrono::high_resolution_clock::now();
float time = std::chrono::duration<float>(currentTime - startTime).count();
vkCmdPushConstants(commandBuffer, particlePipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(float), &time);

// Draw Particles
vkCmdDrawIndexed(commandBuffer, particleIndexCount, 1, 0, 0, 0);

vkCmdEndRendering(commandBuffer);
```

Finally I created two shaders, one for its vertex and one for its fragments, then I compiled them in order to have them working.

```glsl
#version 450

layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in float fragT; // Lifecycle 0.0 (bottom) to 1.0 (top)

layout(location = 0) out vec4 outColor;

void main() {
    // 1. Circular shape logic
    vec2 coord = fragTexCoord * 2.0 - 1.0;
    float dist = length(coord);
    if (dist > 1.0) discard;

    // 2. Define Color Palette
    vec3 colorWhite  = vec3(1.0, 1.0, 1.0); // Hottest base
    vec3 colorYellow = vec3(1.0, 0.8, 0.1); // Body
    vec3 colorRed    = vec3(1.0, 0.1, 0.0); // Top flames
    vec3 colorGrey   = vec3(0.2, 0.2, 0.2); // Smoke

    // 3. Calculate color based on Vertical Lifecycle (fragT)
    vec3 finalColor;

    if (fragT < 0.3) {
        // Bottom: Mix White -> Yellow
        finalColor = mix(colorWhite, colorYellow, fragT / 0.3);
    } else if (fragT < 0.6) {
        // Middle: Mix Yellow -> Red
        finalColor = mix(colorYellow, colorRed, (fragT - 0.3) / 0.3);
    } else {
        // Top: Mix Red -> Grey Smoke
        finalColor = mix(colorRed, colorGrey, (fragT - 0.6) / 0.4);
    }

    // 4. Alpha/Transparency
    // Fade out at the very top (Lifecycle) AND at the edges of the circle (dist)
    float alpha = 1.0 - fragT; // Fade vertically
    alpha = alpha * (1.0 - dist * dist); // Fade radially (squared for softer edge)

    outColor = vec4(finalColor, alpha);
}
```

```glsl
#version 450

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec3 lightPos1;
    vec3 lightColor1;
    vec3 lightPos2;
    vec3 lightColor2;
    vec3 eyePos;
} ubo;

// We will use a Push Constant to pass the Time
layout(push_constant) uniform PushConsts {
    float time;
} pushConsts;

// Input: x,y = quad corners (-1 to 1), z = unique seed (0 to 1)
layout(location = 0) in vec3 inPosition;

layout(location = 0) out vec2 fragTexCoord;
layout(location = 1) out float fragT; // Lifecycle of particle (0 to 1)

// Settings
float particleSpeed = 0.5;
float particleSpread = 1.5;
float particleSize = 0.5;

// ... (Keep existing variables) ...

void main() {
    // 1. Calculate "t" (Lifecycle)
    // Using fract() ensures the particle loops back to the base after reaching 1.0
    float t = fract(inPosition.z + particleSpeed * pushConsts.time);
    fragT = t;

    // 2. Physics
    vec3 centerPos;
    centerPos.y = 2.5 * t; // Move up slightly faster

    // --- FIX IS HERE ---
    // OLD: ... * (1.0 - t)); // Starts wide, ends narrow
    // NEW: ... * t);         // Starts narrow, ends wide (Bonfire shape)

    // We add a small constant (0.2) so the base isn't a single infinite point
    float spreadFactor = (t * 0.5 + 0.2);

    centerPos.x = particleSpread * spreadFactor * (sin(t * 10.0 + inPosition.z * 20.0));
    centerPos.z = particleSpread * spreadFactor * (cos(t * 5.0 + inPosition.z * 10.0));
    // ----------------------

    // 3. Billboarding (Keep exactly as is)
    // Extract camera right and up vectors from the view matrix
    vec3 cameraRight = vec3(ubo.view[0][0], ubo.view[1][0], ubo.view[2][0]);
    vec3 cameraUp    = vec3(ubo.view[0][1], ubo.view[1][1], ubo.view[2][1]);

    // Optional: Make particles smaller as they die to simulate burning out
    // float currentSize = particleSize * (1.0 - t * 0.5);
    vec3 offset = (cameraRight * inPosition.x + cameraUp * inPosition.y) * particleSize;

    vec3 finalPos = centerPos + offset;

    // Standard projection and view transform
    gl_Position = ubo.proj * ubo.view * vec4(finalPos, 1.0);
    
    // Convert -1..1 corners to 0..1 texture coordinates
    fragTexCoord = inPosition.xy * 0.5 + 0.5;
}
```

![Compiled particle system rendering](<../markdown-resources/Complete Real Time Graphics Lab Book (1 - 8)/img7-5-shaders.png> "Compiled particle system rendering")]]

At the end I got the cone effect of fire I was looking for for the particle system.

![Particle system rendering fire effect](<../markdown-resources/Complete Real Time Graphics Lab Book (1 - 8)/img7-5.png> "Particle system rendering fire effect")]

- Test data
N/A

- Sample output
A cylinder rendered with the createCylinder function.

- Reflection

    - *Reflect on what you have learnt from this exercise.*
      How to implement the partycle system, and as with the skybox the changes necessary to render them wihtin Vulkan..

    - *Did you make any mistakes?*
      Yes, originally it went upside down the generation of particles.

    - *In what way has your knowledge improved?*
      In that I can now understand the process to rendergin particles.

- Questions

    - *Is there anything you would like to ask?*
      No.

### Final reflection
These couple of exercises helped me out to understand with more precision what Do i need to do every time I want to compile new shaders or add new graphics pipelines for the different kinds of images needed.

---

## Lab book Chapter 8: Renderable Texture & Post processing

Welcome to the next major step in advanced rendering. So far, we have rendered all of our scenes directly to the **swapchain** (the images presented to the screen). This lab introduces Post-Processing, a powerful technique where we first render our entire 3D scene to an intermediate, off-screen texture.

Once the scene is rendered to a texture, we can apply 2D image-processing effects before it appears on the screen. This is achieved by drawing a full-screen quad textured with the
rendered scene. In the quad’s fragment shader, we can implement effects such as blur, colour tinting, edge detection, or the glow (bloom) effect featured in this lab. This "render to-texture" workflow is the foundation for almost all modern visual effects.

### Exercise 1: RENDER-TO-TEXTURE ON A CUBE

#### Question
This exercise is split into two parts. First, you will implement the depth buffer itself.
Second, you will use it to observe how depth testing and writing are controlled.
1. Goal: Render your 3D cube (with vertex colors) to an off-screen texture. Then, in a second pass, render the same 3D cube to the screen, using the off-screen texture as its new surface texture.
2. Implementation:
    1. **C++ (Create Off-screen Resources)**:
        1. Create a new VkImage (offscreenImage), VkDeviceMemory, (offscreenImageMemory), and VkImageView (offscreenImageView).
        2. When creating offscreenImage, use your swapchain's format and extent, and set usage flags: `VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT`
        3. Create a new VkSampler (offscreenSampler).
    2. **C++ (Create Post-Process Pipeline)**:
        1. Your existing graphicsPipeline (e.g., from you have a achieved in Lab 5 on texture mapping) will be used for Pass 1 (rendering the cube object to the texture).
        2. Create a new VkDescriptorSetLayout. It needs two bindings:
            1. binding = 0: VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER (for the UBO)
            2. binding = 1: VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER (for the offscreenImage)
            3. Create a new VkPipeline: VkPipelineLayout, and VkPipeline. This pipeline will be almost identical to the existing graphicsPipeline, but it will use the new VkPipelineLayout and new shaders.
    3. **C++ (Create Descriptors)**:
        1. Create a VkDescriptorPool and VkDescriptorSet (texturedCubeDescriptorSet) for the new pipeline.
        2. Update this descriptor set to bind both the uniformBuffers[i] (at binding 0) and the offscreenSampler/offscreenImageView (at binding 1).
    4. **GLSL (Shaders for Pass 1 - No Change)**:
        1. vert.spv (from your graphicsPipeline).
        2. frag.spv (from your graphicsPipeline).
    5. **GLSL (Shaders for Pass 2 - Modified)**:
        1. textured_cube.vert: This will be your main vertex shader, vert.spv, modified to pass texture coordinates.
           ```glsl
           #version 450
           layout(binding = 0) uniform UniformBufferObject {
                mat4 model;
                mat4 view;  
                mat4 proj;
           } ubo;

            layout(location = 0) in vec3 inPosition;
           // ... ... ..
           layout(location = 1) out vec2 fragTexCoord; // New
           void main() {
              //... ... ...
              fragTexCoord = inTexCoord; // Pass texcoord
           }
           ```
      2.texture_map.frag: This is the new fragment shader for your textured cube using the texture created in the first pass.
           ```glsl 
           #version 450
           //... ...  
           layout(location = 1) in vec2 fragTexCoord;
           layout(binding = 1) uniform sampler2D sceneTexture; // From Pass 1
           layout(location = 0) out vec4 outColor;
           void main() {
               outColor = texture(sceneTexture, fragTexCoord);
           }
           ```
    6. **C++ (Update recordCommandBuffer)**:
        1. Pass 1: Render Scene to Texture:
          1. Insert barrier to transition offscreenImage from UNDEFINED to COLOR_ATTACHMENT.
          2. Create VkRenderingAttachmentInfo (colorAttachmentInfo) pointing to offscreenImageView.
          3. Create VkRenderingInfo (sceneRenderingInfo) using colorAttachmentInfo and your depthAttachmentInfo.
          4. vkCmdBeginRendering(commandBuffer, &sceneRenderingInfo);
          5. Bind your original 3D scene pipeline and descriptor sets.
          6. Bind vertex and index buffers.  
          7. vkCmdDrawIndexed(...);
          8. vkCmdEndRendering(commandBuffer);
        2. Pass 1 -> 2 Barrier:
          1. Insert barrier to transition offscreenImage from COLOR_ATTACHMENT to SHADER_READ_ONLY. This is mandatory.
        3. Pass 2: Render Textured Cube to Screen:
          1. Insert barrier to transition swapchain image from UNDEFINED to COLOR_ATTACHMENT.
          2. Create VkRenderingAttachmentInfo (swapchainAttachmentInfo) pointing to the current swapchain image view.
          3. Create VkRenderingInfo (finalRenderingInfo) using swapchainAttachmentInfo and your depthAttachmentInfo (clear the depth buffer again).  
          4. vkCmdBeginRendering(commandBuffer, &finalRenderingInfo);
          5. Bind your new texturedCubePipeline and texturedCubeDescriptorSet.
          6. Bind vertex and index buffers again.
          7. vkCmdDrawIndexed(...);
          8. vkCmdEndRendering(commandBuffer)
        4. Final Barrier:
          1. Transition swapchain image to PRESENT_SRC_KHR.
    7. **C++ (Cleanup)**: Remember to destroy the new texturedCubePipeline, texturedCubeLayout, and related descriptor resources
3. Expected Outcome: You will see a textured cube. The texture on the cube will be a "snapshot" of the vertex-colored cube rendered in Pass 1. If the cube is rotating, the texture will appear to "swim" or "slide" across the surface in a disorienting but correct way. If you stop the rotation, the texture on the cube will be a static image of itself

![Example of render-to-texture on cube](<../markdown-resources/Complete Real Time Graphics Lab Book (1 - 8)/img8-1-example.png> "Example of render-to-texture on cube")]]

#### Solution
For this solution, as always when doing a two or more graphic “trick” I had to create new class members and functions in order to implment and create instances of the class
members and create the gaphics pipeline along with the record of the commands and due changes in the clean up

```cpp
// --- Off-screen Resources (Pass 1 Output) ---
VkImage offscreenImage;
VkDeviceMemory offscreenImageMemory;
VkImageView offscreenImageView;
VkSampler offscreenSampler;

// --- Textured Cube Resources (Pass 2 Pipeline) ---
VkDescriptorSetLayout texturedCubeDescriptorSetLayout;
VkPipelineLayout texturedCubePipelineLayout;
VkPipeline texturedCubePipeline;
std::vector<VkDescriptorSet> texturedCubeDescriptorSets;
```

```cpp
/* Off-screen Resources */
void createOffScreenResources();
void createTexturedCubePipeline();
void createTexturedCubeDescriptorSets();
```

After creating the declaration of the three functions that I’ll be using in the post-processing
- CreateOffScreenResrouces: Creates instances of the class members created.
- CreateTexturedCubePipeline: Graphics pipeline for the textured cube, this one should be in a render pass, that must be made after the first one that rendered the scene.
- CreateTexturedCubeDescriptorSets: Creates the descriptor sets for the use of the information.

```cpp
// --- Offscreen Implementation ---
void HelloTriangleApplication::createOffscreenResources() {
    // 1. Create Image
    // We match the SwapChain format and extent so it fits the screen
    createImage(
        swapChainExtent.width, swapChainExtent.height,
        swapChainImageFormat,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, // Important flags!
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        offscreenImage,
        offscreenImageMemory
    );

    // 2. Create Image View
    offscreenImageView = createImageView(offscreenImage, swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);

    // 3. Create Sampler
    // We use a standard linear sampler to read the texture later
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.anisotropyEnable = VK_FALSE; // Blur/Post-process usually doesn't need anisotropy
    samplerInfo.maxAnisotropy = 1.0f;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

    if (vkCreateSampler(device, &samplerInfo, nullptr, &offscreenSampler) != VK_SUCCESS) {
        throw std::runtime_error("failed to create offscreen sampler!");
    }
}
```

```cpp
void HelloTriangleApplication::createTexturedCubePipeline() {
    // 1. Descriptor Set Layout
    VkDescriptorSetLayoutBinding uboLayoutBinding{};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutBinding samplerLayoutBinding{};
    samplerLayoutBinding.binding = 1;
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    std::array<VkDescriptorSetLayoutBinding, 2> bindings = { uboLayoutBinding, samplerLayoutBinding };
    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &texturedCubeDescriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create textured cube descriptor set layout!");
    }

    // 2. Pipeline Layout
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &texturedCubeDescriptorSetLayout;

    if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &texturedCubePipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create textured cube pipeline layout!");
    }

    // 3. Shaders
    auto vertShaderCode = readFile("shaders/textured_cube.vert.spv");
    auto fragShaderCode = readFile("shaders/texture_map.frag.spv");

    VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
    VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

    // 4. Vertex Input (Standard Vertex)
    auto bindingDescription = Vertex::getBindingDescription();
    auto attributeDescriptions = Vertex::getAttributeDescriptions();

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    // 5. Rasterizer (Standard CULL BACK)
    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    // 6. Depth Stencil (Standard LESS)
    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    // 7. Dynamic States
    std::vector<VkDynamicState> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    // 8. Modern Rendering Info
    VkPipelineRenderingCreateInfo renderingCreateInfo{};
    renderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    renderingCreateInfo.colorAttachmentCount = 1;
    renderingCreateInfo.pColorAttachmentFormats = &swapChainImageFormat;
    renderingCreateInfo.depthAttachmentFormat = depthFormat;

    // 9. Graphics Pipeline Creation
    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.pNext = &renderingCreateInfo;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = texturedCubePipelineLayout;

    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &texturedCubePipeline) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create textured cube graphics pipeline!");
    }

    // Clean up temporary shader modules
    vkDestroyShaderModule(device, fragShaderModule, nullptr);
    vkDestroyShaderModule(device, vertShaderModule, nullptr);
}
```

As we know: The descriptor set layout specifies the types of resources that are going to be accessed by the pipeline, just like a render pass specifies the types of attachments that will be accessed. A descriptor set specifies the actual buffer or image resources that will be bound to the descriptors, just like a framebuffer specifies the actual image views to bind to render pass attachments.

Thus, we create the bindings to re-use the main UBO and to get the offscreen image.

```cpp
void HelloTriangleApplication::createTexturedCubeDescriptorSets() {
    std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, texturedCubeDescriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    allocInfo.pSetLayouts = layouts.data();

    texturedCubeDescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
    if (vkAllocateDescriptorSets(device, &allocInfo, texturedCubeDescriptorSets.data()) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate textured cube descriptor sets!");
    }

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        // Binding 0: UBO (Reuse main UBO)
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = uniformBuffers[i];
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(UniformBufferObject);

        // Binding 1: Offscreen Image
        VkDescriptorImageInfo imageInfo{};
        // Note: We will transition the image to this state in drawFrame before sampling
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL; 
        imageInfo.imageView = offscreenImageView;
        imageInfo.sampler = offscreenSampler;

        std::array<VkWriteDescriptorSet, 2> descriptorWrites{};

        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = texturedCubeDescriptorSets[i];
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo = &bufferInfo;

        descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstSet = texturedCubeDescriptorSets[i];
        descriptorWrites[1].dstBinding = 1;
        descriptorWrites[1].dstArrayElement = 0;
        descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].pImageInfo = &imageInfo;

        vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
    }
}
```

On recordCommandBuffer we must add the commands needed to render the the VkImages with the first render pass image into the textured cube.

```cpp
// =============================================================================
// BARRIER: Transition Offscreen Image to SHADER_READ_ONLY
// =============================================================================
VkImageMemoryBarrier2 barrierToShaderRead{};
barrierToShaderRead.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
barrierToShaderRead.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
barrierToShaderRead.srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
barrierToShaderRead.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
barrierToShaderRead.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
barrierToShaderRead.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
barrierToShaderRead.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
barrierToShaderRead.image = offscreenImage;
barrierToShaderRead.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

VkDependencyInfo dependencyToShaderRead{};
dependencyToShaderRead.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
dependencyToShaderRead.imageMemoryBarrierCount = 1;
dependencyToShaderRead.pImageMemoryBarriers = &barrierToShaderRead;
vkCmdPipelineBarrier2(commandBuffer, &dependencyToShaderRead);

// =============================================================================
// PASS 2: Render Textured Cube to Swapchain
// =============================================================================

// 1. Transition Swapchain to COLOR_ATTACHMENT
VkImageMemoryBarrier2 barrierToSwapchain{};
barrierToSwapchain.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
barrierToSwapchain.srcStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
barrierToSwapchain.srcAccessMask = 0;
barrierToSwapchain.dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
barrierToSwapchain.dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
barrierToSwapchain.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
barrierToSwapchain.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
barrierToSwapchain.image = swapChainImages[imageIndex];
barrierToSwapchain.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

VkDependencyInfo dependencyToSwapchain{};
dependencyToSwapchain.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
dependencyToSwapchain.imageMemoryBarrierCount = 1;
dependencyToSwapchain.pImageMemoryBarriers = &barrierToSwapchain;
vkCmdPipelineBarrier2(commandBuffer, &dependencyToSwapchain);

// 2. Attachment Info for Swapchain
VkRenderingAttachmentInfo swapchainAttachment{};
swapchainAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
swapchainAttachment.imageView = swapChainImageViews[imageIndex];
swapchainAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
swapchainAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
swapchainAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
swapchainAttachment.clearValue.color = { 0.0f, 0.0f, 0.0f, 1.0f };

// Clear depth again for the second pass!
depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;

VkRenderingInfo finalRenderingInfo{};
finalRenderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
finalRenderingInfo.renderArea = { {0, 0}, swapChainExtent };
finalRenderingInfo.layerCount = 1;
finalRenderingInfo.colorAttachmentCount = 1;
finalRenderingInfo.pColorAttachments = &swapchainAttachment;
finalRenderingInfo.pDepthAttachment = &depthAttachment;

// 3. Render the Textured Cube
vkCmdBeginRendering(commandBuffer, &finalRenderingInfo);

vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, texturedCubePipeline);

vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

// Bind the new descriptor set that contains the offscreen texture!
vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, texturedCubePipelineLayout, 0, 1, &texturedCubeDescriptorSets[currentFrame], 0, nullptr);

// Draw the cube (meshBuffers[0])
VkBuffer vBuffers2[] = { meshBuffers[0].vertexBuffer };
vkCmdBindVertexBuffers(commandBuffer, 0, 1, vBuffers2, offsets);
vkCmdBindIndexBuffer(commandBuffer, meshBuffers[0].indexBuffer, 0, VK_INDEX_TYPE_UINT32);

vkCmdDrawIndexed(commandBuffer, meshBuffers[0].indexCount, 1, 0, 0, 0);

vkCmdEndRendering(commandBuffer);
```

As we know, we should also add the cleanup of the swapchain and the recreation of it for the re-rendering each frame.

```cpp
void HelloTriangleApplication::recreateSwapChain() {
    int width = 0, height = 0;
    glfwGetFramebufferSize(window, &width, &height);
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(window, &width, &height);
        glfwWaitEvents();
    }

    vkDeviceWaitIdle(device);

    cleanupSwapChain();

    createSwapChain();
    createImageViews();
    createDepthResources();

    // --- LAB 8: RECREATE OFFSCREEN RESOURCES ---
    createOffscreenResources();
    // -------------------------------------------
}

void HelloTriangleApplication::cleanupSwapChain() {
    // 1. Destroy Swapchain Image Views
    for (auto imageView : swapChainImageViews) {
        vkDestroyImageView(device, imageView, nullptr);
    }

    // 2. Destroy Depth Resources
    vkDestroyImageView(device, depthImageView, nullptr);
    vkDestroyImage(device, depthImage, nullptr);
    vkFreeMemory(device, depthImageMemory, nullptr);

    // --- LAB 8: DESTROY OFFSCREEN RESOURCES ---
    // These depend on screen size, so they must go here
    vkDestroySampler(device, offscreenSampler, nullptr);
    vkDestroyImageView(device, offscreenImageView, nullptr);
    vkDestroyImage(device, offscreenImage, nullptr);
    vkFreeMemory(device, offscreenImageMemory, nullptr);
    // -------------------------------------------

    // 3. Destroy Swapchain
    vkDestroySwapchainKHR(device, swapChain, nullptr);
}
```

Because we increased the number of descriptor sets, we also need to increase the pool size.

```cpp
void HelloTriangleApplication::createDescriptorPool() {
    std::array<VkDescriptorPoolSize, 2> poolSizes{};

    // 1. Uniform Buffers: Main(2) + Skybox(2) + TexturedCube(2)
    // We need 3 UBO descriptors per frame in flight
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) * 3; // Increased

    // 2. Image Samplers: Main(5) + Skybox(1) + TexturedCube(1)
    // We need 7 samplers total per frame in flight
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) * 7; // Increased

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();

    // Max sets = Main + Skybox + TexturedCube
    // We need to be able to allocate 3 separate sets per frame
    poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) * 3; // Increased

    if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create descriptor pool!");
    }
}
```

Add to the Vulkan the respective functions in order to have it work.

```cpp
void HelloTriangleApplication::initVulkan() {
    createInstance();
    setupDebugMessenger();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
    createSwapChain();
    createImageViews();
    
    // 1. Core Post-Processing Setup
    createOffscreenResources();
    
    createDescriptorSetLayout();
    createGraphicsPipeline();
    createCommandPool();

    // 2. Resources & Specialized Pipelines
    createCubeMapResources();
    createSkyboxPipeline();

    createParticleResources();
    createParticlePipeline();

    // safe to create early as it defines its own layout
    createTexturedCubePipeline();

    // 3. Texture/Normal/Height Map Setup
    createTextureImage();
    createTextureImageView();
    createTextureSampler();

    createNormalMapImage();
    createNormalMapImageView();
    createNormalMapSampler();

    createHeightMapImage();
    createHeightMapImageView();
    createHeightMapSampler();

    // 4. Mesh and Buffer Allocation
    // std::vector<GeometryUtils::MeshData> meshes = { ... }
    meshBuffers.clear();
    modelMatrices.clear();
    modelMatrices.push_back(glm::mat4(1.0f)); // Center cube

    createUniformBuffers();

    // 5. CRITICAL: Pool must be created BEFORE allocating sets
    createDescriptorPool();

    // 6. Final Allocations
    createDescriptorSets();
    createSkyboxDescriptorSets();
    createTexturedCubeDescriptorSets(); // Now safe after pool exists

    createCommandBuffers();
    createSyncObjects();

    // Log Device Limits
    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(physicalDevice, &properties);
    std::cout << "Max push constant size: " << properties.limits.maxPushConstantsSize << std::endl;
}
```

Now, we actually can work on the shaders that the textured cube graphics pipeline will use, we follow the instructions on the exercise to get the proper render in place.

```glsl
#version 450

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

layout(location = 0) in vec3 inPosition;
// We don't color ro normal for this specific exercise, just TexCoord
layout(location = 3) in vec2 inTexCoord;

layout(location = 1) out vec2 fragTexCoord;

void main() {
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0);
    fragTexCoord = inTexCoord;
}
```

```glsl
#version 450
layout(location = 1) in vec2 fragTexCoord;

// This binding will point to our 'offscreenImage'
layout(binding = 1) uniform sampler2D sceneTexture;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = texture(sceneTexture, fragTexCoord);
}
```

At the end, we got the render of a cube which texture’s the first render pass rendering.

![Render-to-texture on cube](<../markdown-resources/Complete Real Time Graphics Lab Book (1 - 8)/img8-1.png> "Render-to-texture on cube")]

- Test data
N/A

- Sample output
A render of a cube with the previous render as a texture of the cube. I used the previous exercise to make the render of the texture.

- Reflection

    - *Reflect on what you have learnt from this exercise.*
      Well, this time around it was about creating a new render pass completely, along with its graphics pipeline and proper bindings, then record with along at the record buffer.

    - *Did you make any mistakes?*
      No.

    - *In what way has your knowledge improved?*
      In how to implement different render passes that depends on other stuff from the former passes that have already been constructed, in that sense post-processing makes a whole more sense.  
    
- Questions

    - *Is there anything you would like to ask?*
      No.

### Exercise 2: TEXTURE SMOOTHING (BOX BLUR)

### Question
1. Goal: Apply a simple image smoothing algorithm to blur the texture generated in Pass 1 before it is rendered to the screen-aligned quad.
2. Implementation:
    1. New shaders:
        1. fullscreen.vert: Write a new vertex shader to draw a screen-aligned quad.
        2. blur.frag: Write a fragment shader to smooth the texture generated in the first pass.
        ```glsl
        #version 450
        layout(location = 0) in vec2 fragTexCoord;
        layout(binding = 0) uniform sampler2D sceneTexture; // Inputis offscreenImage
        layout(location = 0) out vec4 outColor;
        void main() {
            float stepSize = 3.0; //stepSize = 1.0, 2.0, ...
            vec2 texelSize = stepSize / textureSize(sceneTexture, 0);
            vec4 result = vec4(0.0);
            int boxSize = ... ; // boxSize = 1, 2, ...
            for (int x = - boxSize; x <= boxSize; x++) {
                for (int y = - boxSize; y <= boxSize; y++) {
                    result += texture(sceneTexture, fragTexCoord +
                    vec2(x, y) *texelSize);
                }
            }
            outColor = result /( boxSize * boxSize +1);
        }
        ```
3. Expected Outcome: The cube is rendered to the screen. Its texture is now a blurred version of its vertex-colored self.

![Example of texture smoothing (box blur)](<../markdown-resources/Complete Real Time Graphics Lab Book (1 - 8)/img8-2-example.png> "Example of texture smoothing (box blur")]]

### Solution
For this one I just changed the fragment shader that was using the textured cube according to the exercise’s instructions.

```glsl
#version 450

layout(location = 1) in vec2 fragTexCoord;

// This binding points to our 'offscreenImage' (Pass 1 output)
layout(binding = 1) uniform sampler2D sceneTexture;

layout(location = 0) out vec4 outColor;

void main() {
    // 1. Calculate size of a single texel
    // textureSize returns the width/height of the image in pixels
    // stepSize allows us to jump further to make the blur stronger (e.g., 1.0, 2.0, 3.0)
    float stepSize = 3.0;
    vec2 texelSize = stepSize / vec2(textureSize(sceneTexture, 0));

    vec4 result = vec4(0.0);

    // 2. Convolution Loop (Box Blur)
    // We sample a grid around the center pixel
    // boxSize = 2 means a 5x5 grid (-2 to +2)
    int boxSize = 6;

    for (int x = -boxSize; x <= boxSize; x++) {
        for (int y = -boxSize; y <= boxSize; y++) {
            // Offset coordinate by (x, y) texels
            vec2 offset = vec2(float(x), float(y)) * texelSize;
            result += texture(sceneTexture, fragTexCoord + offset);
        }
    }

    // 3. Average the result
    // Total samples = width * height of the grid
    int totalSamples = (boxSize * 2 + 1) * (boxSize * 2 + 1);

    outColor = result / float(totalSamples);
}
```

At the end I got a cube that was rendering the blurr effect.

![Texture smoothing (box blur)](<../markdown-resources/Complete Real Time Graphics Lab Book (1 - 8)/img8-2.png> "Texture smoothing (box blur")]]

- Test data
N/A

- Sample output
A render blurred cube.

- Reflection

    - *Reflect on what you have learnt from this exercise.*
      That the effect of putting “something” on top of the screen is meant to be a sort of  "looking glass” for the post-processing effects. And that quad should be in its on render pass.

    - *Did you make any mistakes?*
      Yes, while this did work, it didn’t work for the next exercise. Thus, I had to change some stuff around later on.

    - *In what way has your knowledge improved?*
      In the sense that now I know that I should have a render pass for the quad and its effects. For that I changed stuff around in the next exercise.

- Questions

    - *Is there anything you would like to ask?*
      No.

### Exercise 3: SIMPLE GLOW EFFECT
#### Question
1. Goal: Create a "glow" by additively blending the blurred scene back onto the
original, sharp scene.
2. Implementation:
a. Revise your Exercise 2 fragment shader used in the second pass so that it
blends the original texture generated from the first pass with the smoothed
version of the texture.
3. Expected Outcome: A "dreamy" glow effect. The sharp cube will be visible,
additively blended with a full-screen blurred version of itself.

![Example of simple glow effect](<../markdown-resources/Complete Real Time Graphics Lab Book (1 - 8)/img8-3-example.png> "Example of simple glow effect")]]

#### Solution
For this solution I realized something, I need another render pass, have a quad that it’s on the screen as a window and then render on it the blur. So I did that.

To start with this exercise then, I had to create new class members and function declarations on the quad (The third render pass) that will blur and glow the textured cube (second pass).

```cpp
// --- Intermediate Scene Resources (Pass 2 Output) ---
VkImage sceneImage;
VkDeviceMemory sceneImageMemory;
VkImageView sceneImageView;
VkSampler sceneSampler; // We can reuse offscreenSampler, but safer to have its own

// --- Post-Processing Pipeline & Descriptors (Pass 3 Pipeline) ---
VkDescriptorSetLayout postDescriptorSetLayout;
VkPipelineLayout postPipelineLayout;
VkPipeline postPipeline;
std::vector<VkDescriptorSet> postDescriptorSets;
```

```cpp
/* Post-Proces */
void createPostPipeline();
void createPostDescriptorSets();
```

After these declarations, started the implementation of the functions:
- CreatePostPipeline: creates all the configuration in how we will paint the VkImages for the post processing to render. This pipeline will use the sahders of fullscreen.vert that has the vertices for the screen quad and the blur.frag that now render the blum.
- CreatePostDescriptorSets: Has all the information that is passed and used by the render pass.

```cpp
/* Post-process Implementation*/
void HelloTriangleApplication::createPostPipeline() {
    // 1. Descriptor Layout (Simple: just 1 sampler)
    VkDescriptorSetLayoutBinding samplerBinding{};
    samplerBinding.binding = 0; // Let's use binding 0 for simplicity in the post shader
    samplerBinding.descriptorCount = 1;
    samplerBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo layoutInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &samplerBinding;

    if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &postDescriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create post descriptor layout!");
    }

    // 2. Pipeline Layout
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &postDescriptorSetLayout;

    vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &postPipelineLayout);

    // 3. Shaders
    auto vertCode = readFile("shaders/fullscreen.vert.spv");
    auto fragCode = readFile("shaders/blur.frag.spv"); // This will contain the glow logic
    VkShaderModule vertModule = createShaderModule(vertCode);
    VkShaderModule fragModule = createShaderModule(fragCode);

    VkPipelineShaderStageCreateInfo shaderStages[] = {
        {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0, VK_SHADER_STAGE_VERTEX_BIT, vertModule, "main", nullptr},
        {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0, VK_SHADER_STAGE_FRAGMENT_BIT, fragModule, "main", nullptr}
    };

    // 4. States (No Vertex Input, No Culling, No Depth)
    VkPipelineVertexInputStateCreateInfo vertexInput{ VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO }; // Empty
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{ VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkPipelineViewportStateCreateInfo viewportState{ VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterizer{ VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_NONE; // Don't cull the fullscreen triangle
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

    VkPipelineMultisampleStateCreateInfo multisampling{ VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineDepthStencilStateCreateInfo depthStencil{ VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
    depthStencil.depthTestEnable = VK_FALSE; // Always draw on top

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = 0xf;
    colorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlending{ VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    // 5. Dynamic States
    std::vector<VkDynamicState> dynStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    VkPipelineDynamicStateCreateInfo dynamicState{ VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
    dynamicState.dynamicStateCount = (uint32_t)dynStates.size();
    dynamicState.pDynamicStates = dynStates.data();

    // 6. Rendering Info (Output to Screen)
    VkPipelineRenderingCreateInfo renderingInfo{ VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO };
    renderingInfo.colorAttachmentCount = 1;
    renderingInfo.pColorAttachmentFormats = &swapChainImageFormat; // Output to Screen

    // 7. Create Pipeline
    VkGraphicsPipelineCreateInfo pipelineInfo{ VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
    pipelineInfo.pNext = &renderingInfo;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInput;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = postPipelineLayout;

    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &postPipeline) != VK_SUCCESS) {
        throw std::runtime_error("failed to create post pipeline!");
    }

    // Clean up
    vkDestroyShaderModule(device, vertModule, nullptr);
    vkDestroyShaderModule(device, fragModule, nullptr);
}
```

```cpp
void HelloTriangleApplication::createPostDescriptorSets() {
    std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, postDescriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    allocInfo.pSetLayouts = layouts.data();

    postDescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
    if (vkAllocateDescriptorSets(device, &allocInfo, postDescriptorSets.data()) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate post descriptor sets!");
    }

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        // We bind the "sceneImage" (Pass 2 result) here
        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = sceneImageView;
        imageInfo.sampler = sceneSampler; // or offscreenSampler if reused

        VkWriteDescriptorSet descriptorWrite{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
        descriptorWrite.dstSet = postDescriptorSets[i];
        descriptorWrite.dstBinding = 0; // Matches binding=0 in blur.frag
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pImageInfo = &imageInfo;

        vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);
    }
}
```

After that, I had to change stuff in the initVulkan function. As you can see first we create the resources for the skybox and the particle system (for the first render pass), then the texturesCube resources and pipeline. The last part now is the post-processing at least with its descriptor sets, we actually call for its resources earlier.

```cpp
void HelloTriangleApplication::initVulkan() {
    createInstance();
    setupDebugMessenger();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
    createSwapChain();
    createImageViews();
    createOffscreenResources();
    createDescriptorSetLayout();
    createGraphicsPipeline();

    createCommandPool();

    // --- Resources & Pipelines ---
    createCubeMapResources();
    createSkyboxPipeline();

    createParticleResources();
    createParticlePipeline();

    // Move Textured Cube Pipeline here (it's safe to create the pipeline/layout early)
    createTexturedCubePipeline();

    createPostPipeline(); // <--- Needed to create the layout!

    // --- Buffers & Pools ---
    // loadModel(); // (Commented out in your code)

    // Texture/Normal/Height map setup
    createTextureImage();
    createTextureImageView();
    createTextureSampler();

    createNormalMapImage();
    createNormalMapImageView();
    createNormalMapSampler();

    createHeightMapImage();
    createHeightMapImageView();
    createHeightMapSampler();

    // Create a quad mesh using GeometryUtils
    std::vector<GeometryUtils::MeshData> meshes = { ... };

    // Clear global vectors
    meshBuffers.clear();
    modelMatrices.clear();

    // Initial transforms for each cube
    modelMatrices.push_back(glm::mat4(1.0f)); // Center cube

    for (const auto& mesh : meshes) { ... };

    // createVertexBuffer();
    // createIndexBuffer();
    createUniformBuffers();

    // --- CRITICAL: Pool must be created BEFORE allocating sets ---
    createDescriptorPool();

    // --- Allocations (Now safe to allocate) ---
    createDescriptorSets();
    createSkyboxDescriptorSets();
    createTexturedCubeDescriptorSets();

    createPostDescriptorSets(); // <--- Safe now!

    createCommandBuffers();
    createSyncObjects();

    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(physicalDevice, &properties);
    std::cout << "Max push constant size: " << properties.limits.maxPushConstantsSize << std::endl;
}
```

As always, when using and creating new class members, we need to clean them up as well as change the recreation of the swapchain per frame painting.

```cpp
void HelloTriangleApplication::cleanup() {
    // 1. Clean up swapchain-dependent resources (Includes offscreen/scene images)
    cleanupSwapChain();

    // --- PIPELINES & LAYOUTS ---

    // Pass 3: Post Process
    vkDestroyPipeline(device, postPipeline, nullptr);
    vkDestroyPipelineLayout(device, postPipelineLayout, nullptr);
    vkDestroyDescriptorSetLayout(device, postDescriptorSetLayout, nullptr);

    // Pass 2: Textured Cube
    vkDestroyPipeline(device, texturedCubePipeline, nullptr);
    vkDestroyPipelineLayout(device, texturedCubePipelineLayout, nullptr);
    vkDestroyDescriptorSetLayout(device, texturedCubeDescriptorSetLayout, nullptr);

    // Pass 1: Skybox
    vkDestroyPipeline(device, skyboxPipeline, nullptr);
    vkDestroyPipelineLayout(device, skyboxPipelineLayout, nullptr);
    vkDestroyDescriptorSetLayout(device, skyboxDescriptorSetLayout, nullptr);

    // Particles
    vkDestroyPipeline(device, particlePipeline, nullptr);
    vkDestroyPipelineLayout(device, particlePipelineLayout, nullptr);

    // Main Graphics (Pass 1 Solids)
    vkDestroyPipeline(device, graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
    vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);

    // --- BUFFERS ---
    vkDestroyBuffer(device, particleVertexBuffer, nullptr);
    vkFreeMemory(device, particleVertexBufferMemory, nullptr);
    vkDestroyBuffer(device, particleIndexBuffer, nullptr);
    vkFreeMemory(device, particleIndexBufferMemory, nullptr);

    for (const auto& mesh : meshBuffers) {
        vkDestroyBuffer(device, mesh.vertexBuffer, nullptr);
        vkFreeMemory(device, mesh.vertexBufferMemory, nullptr);
        vkDestroyBuffer(device, mesh.indexBuffer, nullptr);
        vkFreeMemory(device, mesh.indexBufferMemory, nullptr);
    }
    meshBuffers.clear();

    vkDestroyBuffer(device, indexBuffer, nullptr);
    vkFreeMemory(device, indexBufferMemory, nullptr);
    vkDestroyBuffer(device, vertexBuffer, nullptr);
    vkFreeMemory(device, vertexBufferMemory, nullptr);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroyBuffer(device, uniformBuffers[i], nullptr);
        vkFreeMemory(device, uniformBuffersMemory[i], nullptr);
    }

    // --- POOLS & SYNC ---
    vkDestroyDescriptorPool(device, descriptorPool, nullptr);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
        vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
        vkDestroyFence(device, inFlightFences[i], nullptr);
    }

    vkDestroyCommandPool(device, commandPool, nullptr);
    vkDestroyDevice(device, nullptr);

    if (enableValidationLayers) {
        DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
    }

    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);

    glfwDestroyWindow(window);
    glfwTerminate();
}
```

```cpp
void HelloTriangleApplication::cleanupSwapChain() {
    // 1. Destroy Swapchain Image Views
    for (auto imageView : swapChainImageViews) {
        vkDestroyImageView(device, imageView, nullptr);
    }

    // 2. Destroy Depth Resources
    vkDestroyImageView(device, depthImageView, nullptr);
    vkDestroyImage(device, depthImage, nullptr);
    vkFreeMemory(device, depthImageMemory, nullptr);

    // 3. Destroy Offscreen Resources (Pass 1 Target)
    vkDestroySampler(device, offscreenSampler, nullptr);
    vkDestroyImageView(device, offscreenImageView, nullptr);
    vkDestroyImage(device, offscreenImage, nullptr);
    vkFreeMemory(device, offscreenImageMemory, nullptr);

    // 4. Destroy Scene Resources (Pass 2 Target)
    vkDestroySampler(device, sceneSampler, nullptr);
    vkDestroyImageView(device, sceneImageView, nullptr);
    vkDestroyImage(device, sceneImage, nullptr);
    vkFreeMemory(device, sceneImageMemory, nullptr);

    // 5. Destroy Swapchain
    vkDestroySwapchainKHR(device, swapChain, nullptr);
}
```

For the record buffer, we also need to change stuff, or a better way to explain it would be to add commands for the render pass 3 (the one for the blur/gloom).

```cpp
// =============================================================================
// PASS 2: Render 3D Cube -> 'sceneImage' (Intermediate)
// =============================================================================

// 1. Barrier: offscreenImage (Write -> Read) AND sceneImage (Undefined -> Write)
VkImageMemoryBarrier2 barriersPass2[2];

// offscreenImage -> Read Only
barriersPass2[0] = barrierToOffscreen; // Reuse struct base
barriersPass2[0].srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
barriersPass2[0].srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
barriersPass2[0].dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
barriersPass2[0].dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
barriersPass2[0].oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
barriersPass2[0].newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
barriersPass2[0].image = offscreenImage;

// sceneImage -> Attachment Write
barriersPass2[1] = barrierToOffscreen; // Reuse struct base
barriersPass2[1].srcStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
barriersPass2[1].srcAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
barriersPass2[1].dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
barriersPass2[1].dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
barriersPass2[1].oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
barriersPass2[1].newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
barriersPass2[1].image = sceneImage;

VkDependencyInfo depPass2{ VK_STRUCTURE_TYPE_DEPENDENCY_INFO };
depPass2.imageMemoryBarrierCount = 2;
depPass2.pImageMemoryBarriers = barriersPass2;
vkCmdPipelineBarrier2(commandBuffer, &depPass2);

// 2. Render Pass 2
VkRenderingAttachmentInfo sceneAttach{ VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
sceneAttach.imageView = sceneImageView;
sceneAttach.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
sceneAttach.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
sceneAttach.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
sceneAttach.clearValue.color = { {0.0f, 0.0f, 0.0f, 1.0f} }; // Black background

// Reuse depth attachment (Clear it again!)
depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;

VkRenderingInfo pass2Info{ VK_STRUCTURE_TYPE_RENDERING_INFO };
pass2Info.renderArea = { {0, 0}, swapChainExtent };
pass2Info.layerCount = 1;
pass2Info.colorAttachmentCount = 1;
pass2Info.pColorAttachments = &sceneAttach;
pass2Info.pDepthAttachment = &depthAttachment;

vkCmdBeginRendering(commandBuffer, &pass2Info);

VkViewport viewport{};
viewport.width = (float)swapChainExtent.width;
viewport.height = (float)swapChainExtent.height;
viewport.maxDepth = 1.0f;
vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

VkRect2D scissor{};
scissor.extent = swapChainExtent;
vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

// Draw the Cube (Textured with Pass 1 result)
vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, texturedCubePipeline);
vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, texturedCubePipelineLayout, 0, 1, &texturedCubeDescriptorSets[currentFrame], 0, nullptr);

// Bind Cube Mesh
VkBuffer cubeVerts[] = { meshBuffers[0].vertexBuffer };
vkCmdBindVertexBuffers(commandBuffer, 0, 1, cubeVerts, offsets);
vkCmdBindIndexBuffer(commandBuffer, meshBuffers[0].indexBuffer, 0, VK_INDEX_TYPE_UINT32);
vkCmdDrawIndexed(commandBuffer, meshBuffers[0].indexCount, 1, 0, 0, 0);

vkCmdEndRendering(commandBuffer);

// =============================================================================
// PASS 3: Fullscreen Glow -> Swapchain
// =============================================================================

// 1. Barrier: sceneImage (Write -> Read) AND Swapchain (Undefined -> Write)
VkImageMemoryBarrier2 barriersPass3[2];

// sceneImage -> Read Only
barriersPass3[0] = barriersPass2[0]; // Reuse structure
barriersPass3[0].image = sceneImage; // Switch to scene image

// Swapchain -> Attachment Write
barriersPass3[1] = barriersPass2[1]; // Reuse structure
barriersPass3[1].srcStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT; // Standard swapchain acquire wait
barriersPass3[1].srcAccessMask = 0;
barriersPass3[1].oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
barriersPass3[1].newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
barriersPass3[1].image = swapChainImages[imageIndex];

VkDependencyInfo depPass3{ VK_STRUCTURE_TYPE_DEPENDENCY_INFO };
depPass3.imageMemoryBarrierCount = 2;
depPass3.pImageMemoryBarriers = barriersPass3;
vkCmdPipelineBarrier2(commandBuffer, &depPass3);

// 2. Render Pass 3
VkRenderingAttachmentInfo swapchainAttach{ VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
swapchainAttach.imageView = swapChainImageViews[imageIndex];
swapchainAttach.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
swapchainAttach.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
swapchainAttach.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
swapchainAttach.clearValue.color = { {0.0f, 0.0f, 0.0f, 1.0f} };

VkRenderingInfo pass3Info{ VK_STRUCTURE_TYPE_RENDERING_INFO };
pass3Info.renderArea = { {0, 0}, swapChainExtent };
pass3Info.layerCount = 1;
pass3Info.colorAttachmentCount = 1;
pass3Info.pColorAttachments = &swapchainAttach;
// No depth attachment needed for fullscreen quad

vkCmdBeginRendering(commandBuffer, &pass3Info);

vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, postPipeline);
vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

// Bind Scene Image (Pass 2 Result)
vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, postPipelineLayout, 0, 1, &postDescriptorSets[currentFrame], 0, nullptr);

// Draw Fullscreen Triangle (3 vertices generated by shader)
vkCmdDraw(commandBuffer, 3, 1, 0, 0);

vkCmdEndRendering(commandBuffer);

// =============================================================================
// Final Barrier: Swapchain -> Present
// =============================================================================
VkImageMemoryBarrier2 barrierPresent{};
barrierPresent.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
barrierPresent.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
barrierPresent.srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
barrierPresent.dstStageMask = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT;
barrierPresent.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
barrierPresent.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
barrierPresent.image = swapChainImages[imageIndex];
barrierPresent.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

VkDependencyInfo depPresent{ VK_STRUCTURE_TYPE_DEPENDENCY_INFO };
depPresent.imageMemoryBarrierCount = 1;
depPresent.pImageMemoryBarriers = &barrierPresent;
vkCmdPipelineBarrier2(commandBuffer, &depPresent);

if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
    throw std::runtime_error("Failed to record command buffer!");
}
```

Because we are also using anew descriptor set, we need to add the size of the descriptor set pool.

```cpp
void HelloTriangleApplication::createDescriptorPool() {
    std::array<VkDescriptorPoolSize, 2> poolSizes{};

    // 1. Uniform Buffers: Main + Skybox + Textured Cube = 3 UBOs per frame
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) * 3;

    // 2. Image Samplers: Main(4) + Skybox(1) + Textured Cube(1) + Post Process(1) = 7 Samplers per frame
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) * 7;

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();

    // Total Sets: Main + Skybox + Textured Cube + Post Process = 4 sets per frame
    poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) * 4;

    if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create descriptor pool!");
    }
}
```

Now I can actually talk about the actual shaders that are used by the quad, in the vertex shader I did create the quad from here, to generate the full-screen triangle that covers the screen in [-1, 1] 

```glsl
#version 450

layout(location = 0) out vec2 fragTexCoord;

void main() {
    // Generates a full-screen triangle that covers the screen [-1, 1]
    // This effectively creates a 2D canvas for our post-processing
    const vec2 pos[3] = vec2[3](
        vec2(-1.0f, -1.0f),
        vec2(3.0f, -1.0f),
        vec2(-1.0f, 3.0f)
    );

    gl_Position = vec4(pos[gl_VertexIndex], 0.0, 1.0);
    fragTexCoord = 0.5 * pos[gl_VertexIndex_] + 0.5;
}
```

And for the gloom, I modified the blur shader to add color.

```glsl
#version 450

layout(location = 0) in vec2 fragTexCoord;
layout(binding = 0) uniform sampler2D cubeTexture;
layout(location = 0) out vec4 outColor;

layout(push_constant) uniform PushConsts {
    float time;
} pushConsts;

// Simple pseudo-random noise function
float rand(vec2 co){
    return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
}

// 2D Noise function for smoother distortion
float noise(vec2 st) {
    vec2 i = floor(st);
    vec2 f = fract(st);
    float a = rand(i);
    float b = rand(i + vec2(1.0, 0.0));
    float c = rand(i + vec2(0.0, 1.0));
    float d = rand(i + vec2(1.0, 1.0));
    vec2 u = f*f*(3.0-2.0*f);
    return mix(a, b, u.x) + (c - a)* u.y * (1.0 - u.x) + (d - b) * u.x * u.y;
}

void main() {
    vec4 original = texture(cubeTexture, fragTexCoord);

    // --- 1. Chaos/Noise Distortion ---
    float noiseVal = noise(fragTexCoord * 10.0 + vec2(0.0, pushConsts.time * 2.0));
    vec2 distortion = vec2(
        (noiseVal - 0.5) * 0.02, // Wiggle X
        (noiseVal - 0.5) * 0.03  // Wiggle Y (Vertical flames)
    );

    // --- 2. Blur Logic ---
    float stepSize = 3.0 + sin(pushConsts.time * 5.0) * 0.5; // Pulsing size
    vec2 texelSize = stepSize / vec2(textureSize(cubeTexture, 0));
    vec4 blurSum = vec4(0.0);
    int boxSize = 6;

    for (int x = -boxSize; x <= boxSize; x++) {
        for (int y = -boxSize; y <= boxSize; y++) {
            vec2 offset = vec2(float(x), float(y)) * texelSize;
            blurSum += texture(cubeTexture, fragTexCoord + offset + distortion);
        }
    }

    float totalSamples = float((boxSize * 2 + 1) * (boxSize * 2 + 1));
    vec4 blurredResult = blurSum / totalSamples;

    // --- 3. Fire Color Gradient (Vertical) ---
    vec3 hotColor = vec3(1.5, 1.2, 0.1); // Bright Yellow
    vec3 coldColor = vec3(1.0, 0.1, 0.0); // Deep Red
    vec3 fireTint = mix(coldColor, hotColor, fragTexCoord.y);

    // --- 4. Intensity Pulse ---
    float pulse = 1.2 + noise(vec2(pushConsts.time * 10.0, 0.0)) * 0.5; // Fast flicker

    vec4 glowEffect = blurredResult * vec4(fireTint, 1.0) * pulse;
    outColor = original + glowEffect;
}
```

At the end we got a gloom blue-ish render.

![Simple glow effect](<../markdown-resources/Complete Real Time Graphics Lab Book (1 - 8)/img8-3.png> "Simple glow effect")]]

- Test data
N/A

- Sample output
A render of a textured cube with a blue-ish glow effect in post-processing.

- Reflection

    - *Reflect on what you have learnt from this exercise.*
      I  learnt about the fact that you can have any number of render passes and that you can chain them together in order to render them on top of another. In this case I had to grab the content of the first pass to render as a texture for the cube in the second render pass and then a third one with the quad in order to render the blur effect correctly with a glow color.

    - *Did you make any mistakes?*
      No.

    - *In what way has your knowledge improved?*
      As in to make the glow effect you brought out the blur from the cube and then distorted with a color and expand upon it.

- Questions

    - *Is there anything you would like to ask?*
      No.

### Exercise 4: OBJECT ON FIRE ANIMATION

#### Question
1. Goal: Modify the glow effect to create a "fire" aura.
2. Implementation:
a. Use the same setup as Exercise 2
i. Capture the target scene object that you want to appear as burning.
ii. Apply a blur effect to the captured texture.
iii. Introduce a timer to animate the blurred texture, creating a dynamic
fire-like behaviour.
iv. Blend the processed (blurred) texture with the original texture, and
render the result onto a screen-aligned quad
3. Expected Outcome: The cube will have a bright, fiery orange/yellow glow, making it
look like it's superheated or on fire. 

![Example of object on fire animation](<../markdown-resources/Complete Real Time Graphics Lab Book (1 - 8)/img8-4-example.png> "Example of object on fire animation")]]

#### Solution
For this solution I had to once again change the recordCommandBuffer function. In this case when writing about the third pass then I had to also bind the image of the render of the pass 2 and push the time to the shader code to have time move like the fire.

```cpp
// =============================================================================
// PASS 3: Fullscreen Glow (Animated) -> Swapchain
// =============================================================================

// 1. Barrier: sceneImage (Write -> Read) AND Swapchain (Undefined -> Write)
VkImageMemoryBarrier2 barriersPass3[2];

// sceneImage -> Read Only
barriersPass3[0] = barriersPass2[0]; // Reuse structure
barriersPass3[0].image = sceneImage; // Switch to scene image

// Swapchain -> Attachment Write
barriersPass3[1] = barriersPass2[1]; // Reuse structure
barriersPass3[1].srcStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT; // Standard swapchain acquire wait
barriersPass3[1].srcAccessMask = 0;
barriersPass3[1].oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
barriersPass3[1].newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
barriersPass3[1].image = swapChainImages[imageIndex];

VkDependencyInfo depPass3{ VK_STRUCTURE_TYPE_DEPENDENCY_INFO };
depPass3.imageMemoryBarrierCount = 2;
depPass3.pImageMemoryBarriers = barriersPass3;
vkCmdPipelineBarrier2(commandBuffer, &depPass3);

// 2. Render Pass 3
VkRenderingAttachmentInfo swapChainAttach{ VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
swapChainAttach.imageView = swapChainImageViews[imageIndex];
swapChainAttach.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
swapChainAttach.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
swapChainAttach.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
swapChainAttach.clearValue.color = { 0.0f, 0.0f, 0.0f, 1.0f };

VkRenderingInfo pass3Info{ VK_STRUCTURE_TYPE_RENDERING_INFO };
pass3Info.renderArea = { {0, 0}, swapChainExtent };
pass3Info.layerCount = 1;
pass3Info.colorAttachmentCount = 1;
pass3Info.pColorAttachments = &swapChainAttach;
// No depth attachment needed for fullscreen quad

vkCmdBeginRendering(commandBuffer, &pass3Info);

vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, postPipeline);
vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

// Bind Scene Image (Pass 2 Result)
vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, postPipelineLayout, 0, 1, &postDescriptorSets[currentFrame], 0, nullptr);

// --- EXERCISE 4: Push Time to Fragment Shader ---
vkCmdPushConstants(commandBuffer, postPipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(float), &time);
// ------------------------------------------------

// Draw Fullscreen Triangle (3 vertices generated by shader)
vkCmdDraw(commandBuffer, 3, 1, 0, 0);

vkCmdEndRendering(commandBuffer);
```

In the shader i used noise to try and break the edges of the blooming effect and make it
seem more like fire, I also added a vertical gradient to make it go from yellow to a more
orange color the more the fire goes up from the cube.

```glsl
#version 450

layout(location = 0) in vec2 fragTexCoord;
layout(binding = 0) uniform sampler2D cubeTexture;
layout(location = 0) out vec4 outColor;

layout(push_constant) uniform PushConsts {
    float time;
} pushConsts;

// 1. Simple pseudo-random noise function
float rand(vec2 co){
    return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
}

// 2. 2D Noise function for smoother distortion
float noise(vec2 st) {
    vec2 i = floor(st);
    vec2 f = fract(st);
    float a = rand(i);
    float b = rand(i + vec2(1.0, 0.0));
    float c = rand(i + vec2(0.0, 1.0));
    float d = rand(i + vec2(1.0, 1.0));
    vec2 u = f*f*(3.0-2.0*f);
    return mix(a, b, u.x) + (c - a)* u.y * (1.0 - u.x) + (d - b) * u.x * u.y;
}

void main() {
    vec4 original = texture(cubeTexture, fragTexCoord);

    // --- 1. Chaos/Noise Distortion ---
    // Instead of simple sine waves, we use noise to break the square edges
    // This makes the blur look like "flames" rather than a "smudge"
    float noiseVal = noise(fragTexCoord * 10.0 + vec2(0.0, pushConsts.time * 2.0));
    vec2 distortion = vec2(
        (noiseVal - 0.5) * 0.02, // Wiggle X
        (noiseVal - 0.5) * 0.03  // Wiggle Y (Vertical flames)
    );

    // --- 2. Blur Logic ---
    float stepSize = 3.0 + sin(pushConsts.time * 5.0) * 0.5; // Pulsing size
    vec2 texelSize = stepSize / vec2(textureSize(cubeTexture, 0));
    vec4 blurSum = vec4(0.0);
    int boxSize = 6;

    // Weighted Blur Loop
    for (int x = -boxSize; x <= boxSize; x++) {
        for (int y = -boxSize; y <= boxSize; y++) {
            vec2 offset = vec2(float(x), float(y)) * texelSize;
            
            // Add distortion to the offset to "scatter" the light
            blurSum += texture(cubeTexture, fragTexCoord + offset + distortion);
        }
    }

    float totalSamples = float((boxSize * 2 + 1) * (boxSize * 2 + 1));
    vec4 blurredResult = blurSum / totalSamples;

    // --- 3. Fire Color Gradient (Vertical) ---
    // Bottom (y=1.0 in UV usually): Yellow/White
    // Top (y=0.0): Red/Orange
    vec3 hotColor = vec3(1.5, 1.2, 0.1); // Bright Yellow
    vec3 coldColor = vec3(1.0, 0.1, 0.0); // Deep Red

    // Assuming UV (0,0) is top-left, so Y=0 is top (Red), Y=1 is bottom (Yellow)
    vec3 fireTint = mix(coldColor, hotColor, fragTexCoord.y);

    // --- 4. Intensity Pulse ---
    float pulse = 1.2 + noise(vec2(pushConsts.time * 10.0, 0.0)) * 0.5; // Fast flicker

    // Combine
    // We multiply the blurred result by our gradient and pulse
    vec4 glowEffect = blurredResult * vec4(fireTint, 1.0) * pulse;

    // Additive Blend
    outColor = original + glowEffect;
}
```

At the end I got the cube in a fire with a wave-effect for the fire while it’s moving.

![Object on fire animation](<../markdown-resources/Complete Real Time Graphics Lab Book (1 - 8)/img8-4.png> "Object on fire animation")]]

- Test data
N/A

- Sample output
A render of cube with a vertical gradient and a wave effect at its borders that were distorted by noise.

- Reflection

    - *Reflect on what you have learnt from this exercise.*
      In the fact that now I know of to pass time values and in practice the vertical gradient for different types of effects.

    - *Did you make any mistakes?*
      No.

    - *In what way has your knowledge improved?*
      In that I had to use a time variable to animate the fire effect, that was something new.

- Questions

    - *Is there anything you would like to ask?*
      No.

### Final Reflection
These exercises helped me understand the concept of post-processing and especially how to bind render passes from one to another and pass information to others so it can be “painted” on top of them to make effects that otherwise wouldn’t be there.