#include "example_base.h"
#include "examples.h"

#include <string.h>

#include "../webgpu/imgui_overlay.h"

/* -------------------------------------------------------------------------- *
 * WebGPU Example - Equirectangular Image
 *
 * This example shows how to render an equirectangular panorama consisting of a
 * single rectangular image. The equirectangular input can be used for a 360
 * degrees viewing experience to achieve more realistic surroundings and
 * convincing real-time effects.
 *
 * Ref:
 * https://www.saschawillems.de/blog/2016/08/13/vulkan-tutorial-on-rendering-a-fullscreen-quad-without-buffers
 * https://onix-systems.com/blog/how-to-use-360-equirectangular-panoramas-for-greater-realism-in-games
 * https://threejs.org/examples/webgl_panorama_equirectangular.html
 * https://www.shadertoy.com/view/4lK3DK
 * http://www.hdrlabs.com/sibl/archive.html
 * -------------------------------------------------------------------------- */

// Uniform buffer block object
static struct {
  WGPUBuffer buffer;
  uint64_t size;
} uniform_buffer_vs = {0};

// Uniform block data - inputs of the shader
static bool shader_inputs_ubo_update_needed = false;
static struct {
  vec2 iResolution; // viewport resolution (in pixels)
  vec4 iMouse;      // mouse pixel coords. xy: current (if MLB down), zw: click
  float iHFovDegrees;   // Horizontal field of view in degrees
  float iVFovDegrees;   // Vertical field of view in degrees
  bool iVisualizeInput; // Show the unprocessed input image
  vec4 padding;         // Padding to reach the minimum binding size of 64 bytes
} shader_inputs_ubo = {
  .iHFovDegrees = 80.0f,
  .iVFovDegrees = 50.0f,
};

// Used for mouse pixel coordinates calculation
static struct {
  vec2 initial_mouse_position;
  vec2 prev_mouse_position;
  vec2 mouse_drag_distance;
  bool dragging;
} mouse_state = {
  .prev_mouse_position = GLM_VEC2_ZERO_INIT,
  .mouse_drag_distance = GLM_VEC2_ZERO_INIT,
  .dragging            = false,
};

// Texture and sampler
static texture_t texture;

// The pipeline layout
static WGPUPipelineLayout pipeline_layout;

// Pipeline
static WGPURenderPipeline pipeline;

// Render pass descriptor for frame buffer writes
static struct {
  WGPURenderPassColorAttachment color_attachments[1];
  WGPURenderPassDescriptor descriptor;
} render_pass;

// The bind group layout
static WGPUBindGroupLayout bind_group_layout;

// The bind group
static WGPUBindGroup bind_group;

// Other variables
static const char* example_title = "Equirectangular Image";
static bool prepared             = false;

static void setup_pipeline_layout(wgpu_context_t* wgpu_context)
{
  WGPUBindGroupLayoutEntry bgl_entries[3] = {
    [0] = (WGPUBindGroupLayoutEntry) {
      //  Binding 1: Fragment shader uniform buffer
      .binding = 0,
      .visibility = WGPUShaderStage_Fragment,
      .buffer = (WGPUBufferBindingLayout) {
        .type             = WGPUBufferBindingType_Uniform,
        .hasDynamicOffset = false,
        .minBindingSize   = sizeof(mat4), // 4x4 matrix
      },
      .sampler = {0},
    },
    [1] = (WGPUBindGroupLayoutEntry) {
      // Binding 1: Fragment shader texture view
      .binding = 1,
      .visibility = WGPUShaderStage_Fragment,
      .texture = (WGPUTextureBindingLayout) {
        .sampleType    = WGPUTextureSampleType_Float,
        .viewDimension = WGPUTextureViewDimension_2D,
        .multisampled  = false,
      },
      .storageTexture = {0},
    },
    [2] = (WGPUBindGroupLayoutEntry) {
      // Binding 2: Fragment shader texture sampler
      .binding = 2,
      .visibility = WGPUShaderStage_Fragment,
      .sampler = (WGPUSamplerBindingLayout){
        .type  = WGPUSamplerBindingType_Filtering,
      },
      .texture = {0},
    },
  };
  bind_group_layout = wgpuDeviceCreateBindGroupLayout(
    wgpu_context->device, &(WGPUBindGroupLayoutDescriptor){
                            .entryCount = (uint32_t)ARRAY_SIZE(bgl_entries),
                            .entries    = bgl_entries,
                          });
  ASSERT(bind_group_layout != NULL)

  // Create the pipeline layout
  pipeline_layout = wgpuDeviceCreatePipelineLayout(
    wgpu_context->device, &(WGPUPipelineLayoutDescriptor){
                            .bindGroupLayoutCount = 1,
                            .bindGroupLayouts     = &bind_group_layout,
                          });
  ASSERT(pipeline_layout != NULL);
}

static void setup_bind_groups(wgpu_context_t* wgpu_context)
{
  WGPUBindGroupEntry bg_entries[3] = {
    [0] = (WGPUBindGroupEntry) {
      .binding = 0,
      .buffer  = uniform_buffer_vs.buffer,
      .offset  = 0,
      .size    = uniform_buffer_vs.size,
    },
    [1] = (WGPUBindGroupEntry) {
      .binding     = 1,
      .textureView = texture.view,
    },
    [2] = (WGPUBindGroupEntry) {
      .binding = 2,
      .sampler = texture.sampler,
    },
  };
  WGPUBindGroupDescriptor bg_desc = {
    .layout     = bind_group_layout,
    .entryCount = (uint32_t)ARRAY_SIZE(bg_entries),
    .entries    = bg_entries,
  };
  bind_group = wgpuDeviceCreateBindGroup(wgpu_context->device, &bg_desc);
  ASSERT(bind_group != NULL)
}

static void prepare_texture(wgpu_context_t* wgpu_context)
{
  const char* file = "textures/Circus_Backstage_8k.jpg";
  texture          = wgpu_create_texture_from_file(wgpu_context, file, NULL);
}

static void setup_render_pass(wgpu_context_t* wgpu_context)
{
  UNUSED_VAR(wgpu_context);

  // Color attachment
  render_pass.color_attachments[0] = (WGPURenderPassColorAttachment) {
      .view       = NULL,
      .loadOp     = WGPULoadOp_Clear,
      .storeOp    = WGPUStoreOp_Store,
      .clearColor = (WGPUColor) {
        .r = 0.0f,
        .g = 0.0f,
        .b = 0.0f,
        .a = 1.0f,
      },
  };

  // Render pass descriptor
  render_pass.descriptor = (WGPURenderPassDescriptor){
    .colorAttachmentCount = 1,
    .colorAttachments     = render_pass.color_attachments,
  };
}

static bool window_resized(wgpu_context_t* wgpu_context)
{
  return ((uint32_t)shader_inputs_ubo.iResolution[0]
          != (uint32_t)wgpu_context->surface.width)
         || ((uint32_t)shader_inputs_ubo.iResolution[1]
             != (uint32_t)wgpu_context->surface.height);
}

static void update_uniform_buffers(wgpu_example_context_t* context)
{
  // iResolution: viewport resolution (in pixels)
  if (window_resized(context->wgpu_context)) {
    shader_inputs_ubo.iResolution[0]
      = (float)context->wgpu_context->surface.width;
    shader_inputs_ubo.iResolution[1]
      = (float)context->wgpu_context->surface.height;
    shader_inputs_ubo_update_needed = true;
  }

  // iMouse: mouse pixel coords. xy: current (if MLB down), zw: click
  if (!mouse_state.dragging && context->mouse_buttons.left) {
    glm_vec2_copy(context->mouse_position, mouse_state.prev_mouse_position);
    mouse_state.dragging = true;
  }
  else if (mouse_state.dragging && context->mouse_buttons.left) {
    glm_vec2_sub(context->mouse_position, mouse_state.prev_mouse_position,
                 mouse_state.mouse_drag_distance);
    glm_vec2_add(shader_inputs_ubo.iMouse, mouse_state.mouse_drag_distance,
                 shader_inputs_ubo.iMouse);
    glm_vec2_copy(context->mouse_position, mouse_state.prev_mouse_position);
    shader_inputs_ubo_update_needed
      = shader_inputs_ubo_update_needed
        || ((fabs(mouse_state.mouse_drag_distance[0]) > 1.0f)
            || (fabs(mouse_state.mouse_drag_distance[1]) > 1.0f));
  }
  else if (mouse_state.dragging && !context->mouse_buttons.left) {
    mouse_state.dragging = false;
  }

  // Map uniform buffer and update when needed
  if (shader_inputs_ubo_update_needed) {
    wgpu_queue_write_buffer(context->wgpu_context, uniform_buffer_vs.buffer, 0,
                            &shader_inputs_ubo, uniform_buffer_vs.size);
    shader_inputs_ubo_update_needed = false;
  }
}

static void prepare_mouse_state(wgpu_context_t* wgpu_context)
{
  glm_vec2_copy(
    (vec2){wgpu_context->surface.width - (wgpu_context->surface.width / 4.0f),
           wgpu_context->surface.height / 2.0f},
    mouse_state.initial_mouse_position);
  glm_vec2_copy((vec4){mouse_state.initial_mouse_position[0],
                       mouse_state.initial_mouse_position[1], 0.0f, 0.0f},
                shader_inputs_ubo.iMouse);
}

static void prepare_uniform_buffers(wgpu_example_context_t* context)
{
  // Create the uniform bind group (note 'rotDeg' is copied here, not bound in
  // any way)
  uniform_buffer_vs.buffer = wgpu_create_buffer_from_data(
    context->wgpu_context, &shader_inputs_ubo, sizeof(shader_inputs_ubo),
    WGPUBufferUsage_Uniform);
  uniform_buffer_vs.size = sizeof(shader_inputs_ubo);

  update_uniform_buffers(context);
}

static void prepare_pipelines(wgpu_context_t* wgpu_context)
{
  // Primitive state
  WGPUPrimitiveState primitive_state_desc = {
    .topology  = WGPUPrimitiveTopology_TriangleList,
    .frontFace = WGPUFrontFace_CCW,
    .cullMode  = WGPUCullMode_Back,
  };

  // Color target state
  WGPUBlendState blend_state                   = wgpu_create_blend_state(false);
  WGPUColorTargetState color_target_state_desc = (WGPUColorTargetState){
    .format    = wgpu_context->swap_chain.format,
    .blend     = &blend_state,
    .writeMask = WGPUColorWriteMask_All,
  };

  // Vertex state
  WGPUVertexState vertex_state_desc = wgpu_create_vertex_state(
                    wgpu_context, &(wgpu_vertex_state_t){
                    .shader_desc = (wgpu_shader_desc_t){
                      // Vertex shader SPIR-V
                      .file = "shaders/equirectangular_image/main.vert.spv",
                    },
                    .buffer_count = 0,
                    .buffers      = NULL,
                  });

  // Fragment state
  WGPUFragmentState fragment_state_desc = wgpu_create_fragment_state(
                    wgpu_context, &(wgpu_fragment_state_t){
                    .shader_desc = (wgpu_shader_desc_t){
                      // Fragment shader SPIR-V
                      .file = "shaders/equirectangular_image/main.frag.spv",
                    },
                    .target_count = 1,
                    .targets      = &color_target_state_desc,
                  });

  // Multisample state
  WGPUMultisampleState multisample_state_desc
    = wgpu_create_multisample_state_descriptor(
      &(create_multisample_state_desc_t){
        .sample_count = 1,
      });

  // Create rendering pipeline using the specified states
  pipeline = wgpuDeviceCreateRenderPipeline(
    wgpu_context->device, &(WGPURenderPipelineDescriptor){
                            .label  = "equirectangular_image_render_pipeline",
                            .layout = pipeline_layout,
                            .primitive   = primitive_state_desc,
                            .vertex      = vertex_state_desc,
                            .fragment    = &fragment_state_desc,
                            .multisample = multisample_state_desc,
                          });

  // Partial cleanup
  WGPU_RELEASE_RESOURCE(ShaderModule, vertex_state_desc.module);
  WGPU_RELEASE_RESOURCE(ShaderModule, fragment_state_desc.module);
}

static int example_initialize(wgpu_example_context_t* context)
{
  if (context) {
    prepare_texture(context->wgpu_context);
    prepare_mouse_state(context->wgpu_context);
    prepare_uniform_buffers(context);
    setup_pipeline_layout(context->wgpu_context);
    setup_bind_groups(context->wgpu_context);
    prepare_pipelines(context->wgpu_context);
    setup_render_pass(context->wgpu_context);
    prepared = true;
    return 0;
  }

  return 1;
}

static float clamp_float(float d, float min, float max)
{
  const float t = d < min ? min : d;
  return t > max ? max : t;
}

static void example_on_update_ui_overlay(wgpu_example_context_t* context)
{
  if (imgui_overlay_header("Settings")) {
    if (imgui_overlay_input_float(
          context->imgui_overlay, "Horizontal FOV (degrees)",
          &shader_inputs_ubo.iHFovDegrees, 1.0f, "%.0f")) {
      shader_inputs_ubo.iHFovDegrees
        = clamp_float(shader_inputs_ubo.iHFovDegrees, 10, 1000);
      shader_inputs_ubo_update_needed = true;
    }
    if (imgui_overlay_input_float(
          context->imgui_overlay, "Vertical FOV (degrees)",
          &shader_inputs_ubo.iVFovDegrees, 1.0f, "%.0f")) {
      shader_inputs_ubo.iVFovDegrees
        = clamp_float(shader_inputs_ubo.iVFovDegrees, 10, 1000);
      shader_inputs_ubo_update_needed = true;
    }
    if (imgui_overlay_checkBox(context->imgui_overlay, "Show input",
                               &shader_inputs_ubo.iVisualizeInput)) {
      shader_inputs_ubo_update_needed = true;
    }
  }
}

static WGPUCommandBuffer build_command_buffer(wgpu_context_t* wgpu_context)
{
  // Set target frame buffer
  render_pass.color_attachments[0].view = wgpu_context->swap_chain.frame_buffer;

  // Create command encoder
  wgpu_context->cmd_enc
    = wgpuDeviceCreateCommandEncoder(wgpu_context->device, NULL);

  // Create render pass encoder for encoding drawing commands
  wgpu_context->rpass_enc = wgpuCommandEncoderBeginRenderPass(
    wgpu_context->cmd_enc, &render_pass.descriptor);

  // Bind the rendering pipeline
  wgpuRenderPassEncoderSetPipeline(wgpu_context->rpass_enc, pipeline);

  // Set the bind group
  wgpuRenderPassEncoderSetBindGroup(wgpu_context->rpass_enc, 0, bind_group, 0,
                                    0);

  // Set viewport
  wgpuRenderPassEncoderSetViewport(
    wgpu_context->rpass_enc, 0.0f, 0.0f, (float)wgpu_context->surface.width,
    (float)wgpu_context->surface.height, 0.0f, 1.0f);

  // Set scissor rectangle
  wgpuRenderPassEncoderSetScissorRect(wgpu_context->rpass_enc, 0u, 0u,
                                      wgpu_context->surface.width,
                                      wgpu_context->surface.height);

  // Draw indexed quad
  wgpuRenderPassEncoderDraw(wgpu_context->rpass_enc, 3, 1, 0, 0);

  // End render pass
  wgpuRenderPassEncoderEndPass(wgpu_context->rpass_enc);
  WGPU_RELEASE_RESOURCE(RenderPassEncoder, wgpu_context->rpass_enc)

  // Draw ui overlay
  draw_ui(wgpu_context->context, example_on_update_ui_overlay);

  // Get command buffer
  WGPUCommandBuffer command_buffer
    = wgpu_get_command_buffer(wgpu_context->cmd_enc);
  WGPU_RELEASE_RESOURCE(CommandEncoder, wgpu_context->cmd_enc)

  return command_buffer;
}

static int example_draw(wgpu_example_context_t* context)
{
  wgpu_context_t* wgpu_context = context->wgpu_context;

  // Update the uniform buffers
  update_uniform_buffers(context);

  // Prepare frame
  prepare_frame(context);

  // Command buffer to be submitted to the queue
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
  return example_draw(context);
}

static void example_destroy(wgpu_example_context_t* context)
{
  camera_release(context->camera);
  wgpu_destroy_texture(&texture);
  WGPU_RELEASE_RESOURCE(Buffer, uniform_buffer_vs.buffer)
  WGPU_RELEASE_RESOURCE(PipelineLayout, pipeline_layout)
  WGPU_RELEASE_RESOURCE(BindGroupLayout, bind_group_layout)
  WGPU_RELEASE_RESOURCE(BindGroup, bind_group)
  WGPU_RELEASE_RESOURCE(RenderPipeline, pipeline)
}

void example_equirectangular_image(int argc, char* argv[])
{
  // clang-format off
  example_run(argc, argv, &(refexport_t){
    .example_settings = (wgpu_example_settings_t){
     .title   = example_title,
     .overlay = true,
     .vsync   = true,
    },
    .example_initialize_func      = &example_initialize,
    .example_render_func          = &example_render,
    .example_destroy_func         = &example_destroy,
  });
  // clang-format on
}
