#include "example_base.h"
#include "examples.h"

#include <string.h>

#include "../webgpu/imgui_overlay.h"

/* -------------------------------------------------------------------------- *
 * WebGPU Example - Cube Reflection
 *
 * This example shows how to create a basic reflection pipeline.
 *
 * Ref:
 * https://dawn.googlesource.com/dawn/+/refs/heads/main/examples/CubeReflection.cpp
 * -------------------------------------------------------------------------- */

// Index buffer
static struct indices_t {
  WGPUBuffer buffer;
  uint32_t count;
} indices = {0};

// Cube vertex buffer
static struct cube_vertices_t {
  WGPUBuffer buffer;
  uint32_t count;
} cube_vertices = {0};

// Plane vertex buffer
static struct plane_vertices_t {
  WGPUBuffer buffer;
  uint32_t count;
} plane_vertices = {0};

static struct camera_data_t {
  mat4 view;
  mat4 proj;
} camera_data = {0};

// Other buffers
static WGPUBuffer camera_buffer;
static WGPUBuffer transform_buffer[2];

// Bind groups stores the resources bound to the binding points in a shader
static WGPUBindGroup bind_group[2];
static WGPUBindGroupLayout bind_group_layout;

// The pipeline layout
static WGPUPipelineLayout pipeline_layout;

// Pipelines
static WGPURenderPipeline pipeline;
static WGPURenderPipeline plane_pipeline;
static WGPURenderPipeline reflection_pipeline;

// Render pass descriptor for frame buffer writes
static WGPURenderPassColorAttachment rp_color_att_descriptors[1];
static WGPURenderPassDescriptor render_pass_desc;

// Render state
static struct render_state_t {
  uint32_t a;
  float b;
} render_state = {0};

// Other variables
static const char* example_title = "Cube Reflection";
static bool prepared             = false;

static void prepare_buffers(wgpu_context_t* wgpu_context)
{
  // Index buffer
  {
    static const uint32_t index_data[6 * 6] = {
      0,  1,  2,  0,  2,  3, //

      4,  5,  6,  4,  6,  7, //

      8,  9,  10, 8,  10, 11, //

      12, 13, 14, 12, 14, 15, //

      16, 17, 18, 16, 18, 19, //

      20, 21, 22, 20, 22, 23, //
    };
    indices.count              = (uint32_t)ARRAY_SIZE(index_data);
    uint32_t index_buffer_size = indices.count * sizeof(uint32_t);
    indices.buffer             = wgpu_create_buffer_from_data(
      wgpu_context, index_data, index_buffer_size, WGPUBufferUsage_Index);
  }

  // Cube vertices data
  {
    static const float vertex_data[6 * 4 * 6] = {
      -1.f, -1.f, 1.f,  1.f, 0.f, 0.f, 1.f,  -1.f, 1.f,  1.f, 0.f, 0.f, //
      1.f,  1.f,  1.f,  1.f, 0.f, 0.f, -1.f, 1.f,  1.f,  1.f, 0.f, 0.f, //

      -1.f, -1.f, -1.f, 1.f, 1.f, 0.f, -1.f, 1.f,  -1.f, 1.f, 1.f, 0.f, //
      1.f,  1.f,  -1.f, 1.f, 1.f, 0.f, 1.f,  -1.f, -1.f, 1.f, 1.f, 0.f, //

      -1.f, 1.f,  -1.f, 1.f, 0.f, 1.f, -1.f, 1.f,  1.f,  1.f, 0.f, 1.f, //
      1.f,  1.f,  1.f,  1.f, 0.f, 1.f, 1.f,  1.f,  -1.f, 1.f, 0.f, 1.f, //

      -1.f, -1.f, -1.f, 0.f, 1.f, 0.f, 1.f,  -1.f, -1.f, 0.f, 1.f, 0.f, //
      1.f,  -1.f, 1.f,  0.f, 1.f, 0.f, -1.f, -1.f, 1.f,  0.f, 1.f, 0.f, //

      1.f,  -1.f, -1.f, 0.f, 1.f, 1.f, 1.f,  1.f,  -1.f, 0.f, 1.f, 1.f, //
      1.f,  1.f,  1.f,  0.f, 1.f, 1.f, 1.f,  -1.f, 1.f,  0.f, 1.f, 1.f, //

      -1.f, -1.f, -1.f, 1.f, 1.f, 1.f, -1.f, -1.f, 1.f,  1.f, 1.f, 1.f, //
      -1.f, 1.f,  1.f,  1.f, 1.f, 1.f, -1.f, 1.f,  -1.f, 1.f, 1.f, 1.f, //
    };
    cube_vertices.count         = (uint32_t)ARRAY_SIZE(vertex_data);
    uint32_t vertex_buffer_size = cube_vertices.count * sizeof(float);
    cube_vertices.buffer        = wgpu_create_buffer_from_data(
      wgpu_context, vertex_data, vertex_buffer_size, WGPUBufferUsage_Vertex);
  }

  // Plane vertice data
  {
    static const float plane_data[6 * 4] = {
      -2.f, -1.f, -2.f, 0.5f, 0.5f, 0.5f, 2.f,  -1.f, -2.f, 0.5f, 0.5f, 0.5f, //
      2.f,  -1.f, 2.f,  0.5f, 0.5f, 0.5f, -2.f, -1.f, 2.f,  0.5f, 0.5f, 0.5f, //
    };
    plane_vertices.count        = (uint32_t)ARRAY_SIZE(plane_data);
    uint32_t vertex_buffer_size = cube_vertices.count * sizeof(float);
    plane_vertices.buffer       = wgpu_create_buffer_from_data(
      wgpu_context, plane_data, vertex_buffer_size, WGPUBufferUsage_Vertex);
  }
}

static void prepare_uniform_buffers(wgpu_context_t* wgpu_context)
{
  // Camera data buffer
  {
    WGPUBufferDescriptor buffer_desc = {
      .usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform,
      .size  = sizeof(camera_data),
    };
    camera_buffer = wgpuDeviceCreateBuffer(wgpu_context->device, &buffer_desc);
  }

  // Camera projection matrix
  glm_perspective(glm_rad(45.0f), 1.f, 1.0f, 100.0f, camera_data.proj);

  // Transform buffers
  {
    mat4 transform      = GLM_MAT4_IDENTITY_INIT;
    transform_buffer[0] = wgpu_create_buffer_from_data(
      wgpu_context, &transform, sizeof(mat4), WGPUBufferUsage_Uniform);

    glm_translate(transform, (vec3){0.f, -2.f, 0.f});
    transform_buffer[1] = wgpu_create_buffer_from_data(
      wgpu_context, &transform, sizeof(mat4), WGPUBufferUsage_Uniform);
  }
}

static void setup_render_pass(wgpu_context_t* wgpu_context)
{
  // Color attachment
  rp_color_att_descriptors[0] = (WGPURenderPassColorAttachment) {
      .view       = NULL, // attachment is acquired in render loop.
      .loadOp     = WGPULoadOp_Clear,
      .storeOp    = WGPUStoreOp_Store,
      .clearColor = (WGPUColor) {
        .r = 0.1f,
        .g = 0.2f,
        .b = 0.3f,
        .a = 1.0f,
      },
  };

  // Depth attachment
  wgpu_setup_deph_stencil(wgpu_context, NULL);

  // Render pass descriptor
  render_pass_desc = (WGPURenderPassDescriptor){
    .colorAttachmentCount   = 1,
    .colorAttachments       = rp_color_att_descriptors,
    .depthStencilAttachment = &wgpu_context->depth_stencil.att_desc,
  };
}

static void setup_pipeline_layout(wgpu_context_t* wgpu_context)
{
  // Bind group layout
  WGPUBindGroupLayoutEntry bgl_entries[2] = {
    [0] = (WGPUBindGroupLayoutEntry) {
      // Binding 0: Uniform buffer (Vertex shader) => cameraData
      .binding = 0,
      .visibility = WGPUShaderStage_Vertex,
      .buffer = (WGPUBufferBindingLayout) {
        .type = WGPUBufferBindingType_Uniform,
        .hasDynamicOffset = false,
        .minBindingSize = sizeof(camera_data),
      },
      .sampler = {0},
    },
    [1] = (WGPUBindGroupLayoutEntry) {
      // Binding 1: Uniform buffer (Vertex shader) => modelData
      .binding = 1,
      .visibility = WGPUShaderStage_Vertex,
      .buffer = (WGPUBufferBindingLayout) {
        .type = WGPUBufferBindingType_Uniform,
        .hasDynamicOffset = false,
        .minBindingSize = sizeof(mat4),
      },
      .sampler = {0},
    },
  };
  WGPUBindGroupLayoutDescriptor bgl_desc = {
    .entryCount = 2,
    .entries    = bgl_entries,
  };
  bind_group_layout
    = wgpuDeviceCreateBindGroupLayout(wgpu_context->device, &bgl_desc);
  ASSERT(bind_group_layout != NULL);

  // Pipeline layout
  WGPUPipelineLayoutDescriptor pipeline_layout_desc = {
    .bindGroupLayoutCount = 1,
    .bindGroupLayouts     = &bind_group_layout,
  };
  pipeline_layout = wgpuDeviceCreatePipelineLayout(wgpu_context->device,
                                                   &pipeline_layout_desc);
  ASSERT(pipeline_layout != NULL)
}

static void setup_bind_groups(wgpu_context_t* wgpu_context)
{
  // Bind groups
  for (uint32_t i = 0; i < (uint32_t)ARRAY_SIZE(bind_group); ++i) {
    WGPUBindGroupEntry bg_entries[2] = {
      [0] = {
        .binding = 0,
        .buffer  = camera_buffer,
        .offset  = 0,
        .size    = sizeof(camera_data),
      },
      [1] = {
        .binding = 1,
        .buffer  = transform_buffer[i],
        .offset  = 0,
        .size    = sizeof(mat4),
      },
    };
    WGPUBindGroupDescriptor bg_desc = {
      .layout     = bind_group_layout,
      .entryCount = 2,
      .entries    = bg_entries,
    };
    bind_group[i] = wgpuDeviceCreateBindGroup(wgpu_context->device, &bg_desc);
    ASSERT(bind_group[i] != NULL)
  }
}

// Create the graphics pipeline
static void prepare_pipelines(wgpu_context_t* wgpu_context)
{
  // Primitive state
  WGPUPrimitiveState primitive_state_desc = {
    .topology  = WGPUPrimitiveTopology_TriangleList,
    .frontFace = WGPUFrontFace_CCW,
    .cullMode  = WGPUCullMode_None,
  };

  // Color target state
  WGPUBlendState blend_state                   = wgpu_create_blend_state(true);
  WGPUColorTargetState color_target_state_desc = (WGPUColorTargetState){
    .format    = wgpu_context->swap_chain.format,
    .blend     = &blend_state,
    .writeMask = WGPUColorWriteMask_All,
  };

  // Vertex buffer layout
  WGPU_VERTEX_BUFFER_LAYOUT(
    cube_reflection, 6 * sizeof(float),
    // Attribute location 0: Position
    WGPU_VERTATTR_DESC(0, WGPUVertexFormat_Float32x3, 0),
    // Attribute location 1: Color
    WGPU_VERTATTR_DESC(1, WGPUVertexFormat_Float32x3, 3 * sizeof(float)));

  // Vertex state
  WGPUVertexState vertex_state_desc = wgpu_create_vertex_state(
            wgpu_context, &(wgpu_vertex_state_t){
            .shader_desc = (wgpu_shader_desc_t){
              // Vertex shader SPIR-V
              .file = "shaders/cube_reflection/shader.vert.spv",
            },
            .buffer_count = 1,
            .buffers = &cube_reflection_vertex_buffer_layout,
          });

  // Fragment states
  WGPUFragmentState fragment_state_desc = wgpu_create_fragment_state(
            wgpu_context, &(wgpu_fragment_state_t){
            .shader_desc = (wgpu_shader_desc_t){
              // Fragment shader SPIR-V
              .file = "shaders/cube_reflection/shader.frag.spv",
            },
            .target_count = 1,
            .targets = &color_target_state_desc,
          });
  WGPUFragmentState fragment_state_reflection_desc = wgpu_create_fragment_state(
            wgpu_context, &(wgpu_fragment_state_t){
            .shader_desc = (wgpu_shader_desc_t){
              // Fragment shader SPIR-V
              .file = "shaders/cube_reflection/reflection.frag.spv",
            },
            .target_count = 1,
            .targets = &color_target_state_desc,
          });

  // Multisample state
  WGPUMultisampleState multisample_state_desc
    = wgpu_create_multisample_state_descriptor(
      &(create_multisample_state_desc_t){
        .sample_count = 1,
      });

  // Cube rendering pipeline
  {
    // Depth stencil state
    WGPUDepthStencilState depth_stencil_state_desc
      = wgpu_create_depth_stencil_state(&(create_depth_stencil_state_desc_t){
        .format              = WGPUTextureFormat_Depth24PlusStencil8,
        .depth_write_enabled = true,
      });
    depth_stencil_state_desc.depthCompare = WGPUCompareFunction_Less;

    // Create rendering pipeline using the specified states
    pipeline = wgpuDeviceCreateRenderPipeline(
      wgpu_context->device, &(WGPURenderPipelineDescriptor){
                              .label        = "cube_render_pipeline",
                              .layout       = pipeline_layout,
                              .primitive    = primitive_state_desc,
                              .vertex       = vertex_state_desc,
                              .fragment     = &fragment_state_desc,
                              .depthStencil = &depth_stencil_state_desc,
                              .multisample  = multisample_state_desc,
                            });
  }

  // Plane rendering pipeline
  {
    // Depth stencil state
    WGPUDepthStencilState depth_stencil_state_desc
      = wgpu_create_depth_stencil_state(&(create_depth_stencil_state_desc_t){
        .format              = WGPUTextureFormat_Depth24PlusStencil8,
        .depth_write_enabled = false,
      });
    depth_stencil_state_desc.stencilFront.passOp = WGPUStencilOperation_Replace;
    depth_stencil_state_desc.stencilBack.passOp  = WGPUStencilOperation_Replace;
    depth_stencil_state_desc.depthCompare        = WGPUCompareFunction_Less;

    // Create rendering pipeline using the specified states
    plane_pipeline = wgpuDeviceCreateRenderPipeline(
      wgpu_context->device, &(WGPURenderPipelineDescriptor){
                              .label        = "plane_render_pipeline",
                              .layout       = pipeline_layout,
                              .primitive    = primitive_state_desc,
                              .vertex       = vertex_state_desc,
                              .fragment     = &fragment_state_desc,
                              .depthStencil = &depth_stencil_state_desc,
                              .multisample  = multisample_state_desc,
                            });
  }

  // Cube reflection rendering pipeline
  {
    // Depth stencil state
    WGPUDepthStencilState depth_stencil_state_desc
      = wgpu_create_depth_stencil_state(&(create_depth_stencil_state_desc_t){
        .format              = WGPUTextureFormat_Depth24PlusStencil8,
        .depth_write_enabled = true,
      });
    depth_stencil_state_desc.stencilFront.compare = WGPUCompareFunction_Equal;
    depth_stencil_state_desc.stencilBack.compare  = WGPUCompareFunction_Equal;
    depth_stencil_state_desc.stencilFront.passOp = WGPUStencilOperation_Replace;
    depth_stencil_state_desc.stencilBack.passOp  = WGPUStencilOperation_Replace;
    depth_stencil_state_desc.depthCompare        = WGPUCompareFunction_Less;

    // Create rendering pipeline using the specified states
    reflection_pipeline = wgpuDeviceCreateRenderPipeline(
      wgpu_context->device, &(WGPURenderPipelineDescriptor){
                              .label        = "cube_reflection_render_pipeline",
                              .layout       = pipeline_layout,
                              .primitive    = primitive_state_desc,
                              .vertex       = vertex_state_desc,
                              .fragment     = &fragment_state_reflection_desc,
                              .depthStencil = &depth_stencil_state_desc,
                              .multisample  = multisample_state_desc,
                            });
  }

  // Partial cleanup
  WGPU_RELEASE_RESOURCE(ShaderModule, vertex_state_desc.module);
  WGPU_RELEASE_RESOURCE(ShaderModule, fragment_state_desc.module);
  WGPU_RELEASE_RESOURCE(ShaderModule, fragment_state_reflection_desc.module);
}

static void update_camera_view(wgpu_context_t* wgpu_context)
{
  // Update render state
  render_state.a = (render_state.a + 1) % 256;
  render_state.b += 0.002f;
  if (render_state.b >= 1.0f) {
    render_state.b = 0.0f;
  }

  // Update camera view
  glm_lookat((vec3){8.f * sin(glm_rad(render_state.b * 360.f)),  // x (eye)
                    2.f,                                         // y (eye)
                    8.f * cos(glm_rad(render_state.b * 360.f))}, // z (eye)
             (vec3){0.0f, 0.0f, 0.0f},                           // center
             (vec3){0.0f, 1.0f, 0.0f},                           // up
             camera_data.view);

  // Update uniform buffer
  wgpu_queue_write_buffer(wgpu_context, camera_buffer, 0, &camera_data,
                          sizeof(camera_data));
}

static int example_initialize(wgpu_example_context_t* context)
{
  if (context) {
    prepare_buffers(context->wgpu_context);
    prepare_uniform_buffers(context->wgpu_context);
    setup_pipeline_layout(context->wgpu_context);
    setup_bind_groups(context->wgpu_context);
    prepare_pipelines(context->wgpu_context);
    setup_render_pass(context->wgpu_context);
    prepared = true;
    return 0;
  }

  return 1;
}

static void example_on_update_ui_overlay(wgpu_example_context_t* context)
{
  if (imgui_overlay_header("Settings")) {
    imgui_overlay_checkBox(context->imgui_overlay, "Paused", &context->paused);
  }
}

static WGPUCommandBuffer build_command_buffer(wgpu_context_t* wgpu_context)
{
  rp_color_att_descriptors[0].view = wgpu_context->swap_chain.frame_buffer;

  wgpu_context->cmd_enc
    = wgpuDeviceCreateCommandEncoder(wgpu_context->device, NULL);

  {
    WGPURenderPassEncoder rpass_enc = wgpuCommandEncoderBeginRenderPass(
      wgpu_context->cmd_enc, &render_pass_desc);

    // Render cube
    {
      wgpuRenderPassEncoderSetPipeline(rpass_enc, pipeline);
      wgpuRenderPassEncoderSetBindGroup(rpass_enc, 0, bind_group[0], 0, 0);
      wgpuRenderPassEncoderSetVertexBuffer(rpass_enc, 0, cube_vertices.buffer,
                                           0, WGPU_WHOLE_SIZE);
      wgpuRenderPassEncoderSetIndexBuffer(rpass_enc, indices.buffer,
                                          WGPUIndexFormat_Uint32, 0, 0);
      wgpuRenderPassEncoderDrawIndexed(rpass_enc, 36, 1, 0, 0, 0);
    }

    // Render plane
    {
      wgpuRenderPassEncoderSetStencilReference(rpass_enc, 0x1);
      wgpuRenderPassEncoderSetPipeline(rpass_enc, plane_pipeline);
      wgpuRenderPassEncoderSetBindGroup(rpass_enc, 0, bind_group[0], 0, 0);
      wgpuRenderPassEncoderSetVertexBuffer(rpass_enc, 0, plane_vertices.buffer,
                                           0, WGPU_WHOLE_SIZE);
      wgpuRenderPassEncoderDrawIndexed(rpass_enc, 6, 1, 0, 0, 0);
    }

    // Render cube reflection
    {
      wgpuRenderPassEncoderSetPipeline(rpass_enc, reflection_pipeline);
      wgpuRenderPassEncoderSetBindGroup(rpass_enc, 0, bind_group[1], 0, 0);
      wgpuRenderPassEncoderSetVertexBuffer(rpass_enc, 0, cube_vertices.buffer,
                                           0, WGPU_WHOLE_SIZE);
      wgpuRenderPassEncoderDrawIndexed(rpass_enc, 36, 1, 0, 0, 0);
    }

    wgpuRenderPassEncoderEndPass(rpass_enc);
    WGPU_RELEASE_RESOURCE(RenderPassEncoder, rpass_enc)
  }

  // Draw ui overlay
  draw_ui(wgpu_context->context, example_on_update_ui_overlay);

  // Get command buffer
  WGPUCommandBuffer command_buffer
    = wgpu_get_command_buffer(wgpu_context->cmd_enc);
  ASSERT(command_buffer != NULL)
  WGPU_RELEASE_RESOURCE(CommandEncoder, wgpu_context->cmd_enc)

  return command_buffer;
}

static int example_draw(wgpu_example_context_t* context)
{
  // Prepare frame
  prepare_frame(context);

  // Command buffer to be submitted to the queue
  wgpu_context_t* wgpu_context                   = context->wgpu_context;
  wgpu_context->submit_info.command_buffer_count = 1;
  wgpu_context->submit_info.command_buffers[0]
    = build_command_buffer(context->wgpu_context);

  // Submit to queue
  submit_command_buffers(context);

  // Submit frame
  submit_frame(context);

  return 0;
}

static int example_render(wgpu_example_context_t* context)
{
  if (!prepared) {
    return 1;
  }
  const int draw_result = example_draw(context);
  if (!context->paused) {
    update_camera_view(context->wgpu_context);
  }
  return draw_result;
}

// Clean up used resources
static void example_destroy(wgpu_example_context_t* context)
{
  UNUSED_VAR(context);
  WGPU_RELEASE_RESOURCE(Buffer, indices.buffer)
  WGPU_RELEASE_RESOURCE(Buffer, cube_vertices.buffer)
  WGPU_RELEASE_RESOURCE(Buffer, plane_vertices.buffer)
  WGPU_RELEASE_RESOURCE(Buffer, camera_buffer)
  WGPU_RELEASE_RESOURCE(Buffer, transform_buffer[0])
  WGPU_RELEASE_RESOURCE(Buffer, transform_buffer[1])
  WGPU_RELEASE_RESOURCE(BindGroup, bind_group[0])
  WGPU_RELEASE_RESOURCE(BindGroup, bind_group[1])
  WGPU_RELEASE_RESOURCE(BindGroupLayout, bind_group_layout)
  WGPU_RELEASE_RESOURCE(PipelineLayout, pipeline_layout)
  WGPU_RELEASE_RESOURCE(RenderPipeline, pipeline)
  WGPU_RELEASE_RESOURCE(RenderPipeline, plane_pipeline)
  WGPU_RELEASE_RESOURCE(RenderPipeline, reflection_pipeline)
}

void example_cube_reflection(int argc, char* argv[])
{
  // clang-format off
  example_run(argc, argv, &(refexport_t){
    .example_settings = (wgpu_example_settings_t){
      .title  = example_title,
      .overlay = true,
    },
    .example_initialize_func      = &example_initialize,
    .example_render_func          = &example_render,
    .example_destroy_func         = &example_destroy
  });
  // clang-format on
}
