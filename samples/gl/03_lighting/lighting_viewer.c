/*
 * OpenGL sample 03: draw one mesh with resolved scene lights.
 *
 * This is a small preview shader, not a production renderer. The important
 * part is the AssetKit side: material resolution and ak_lightResolve(PREVIEW).
 */

#include "../gl_sample_common.h"

static void
sample_set_light_uniforms(GLuint program, const SampleGLLight *lights, size_t count) {
  GLint loc;
  size_t i;

  glUseProgram(program);
  loc = glGetUniformLocation(program, "u_light_count");
  glUniform1i(loc, (GLint)count);

  for (i = 0; i < count; i++) {
    char name[64];

    snprintf(name, sizeof(name), "u_lights[%zu].type", i);
    glUniform1i(glGetUniformLocation(program, name), lights[i].type);
    snprintf(name, sizeof(name), "u_lights[%zu].position", i);
    glUniform3fv(glGetUniformLocation(program, name), 1, lights[i].position);
    snprintf(name, sizeof(name), "u_lights[%zu].direction", i);
    glUniform3fv(glGetUniformLocation(program, name), 1, lights[i].direction);
    snprintf(name, sizeof(name), "u_lights[%zu].color", i);
    glUniform3fv(glGetUniformLocation(program, name), 1, lights[i].color);
    snprintf(name, sizeof(name), "u_lights[%zu].intensity", i);
    glUniform1f(glGetUniformLocation(program, name), lights[i].intensity);
    snprintf(name, sizeof(name), "u_lights[%zu].range", i);
    glUniform1f(glGetUniformLocation(program, name), lights[i].range);
    snprintf(name, sizeof(name), "u_lights[%zu].inner_cos", i);
    glUniform1f(glGetUniformLocation(program, name), lights[i].inner_cos);
    snprintf(name, sizeof(name), "u_lights[%zu].outer_cos", i);
    glUniform1f(glGetUniformLocation(program, name), lights[i].outer_cos);
  }
}

int
main(int argc, char **argv) {
  AkDoc *doc;
  AkScene *scene;
  SampleGLMesh mesh;
  SampleGLLight lights[SAMPLE_GL_MAX_LIGHTS];
  size_t light_count;
  GLFWwindow *window;
  GLuint vao;
  GLuint vbo;
  GLuint ebo;
  GLuint tex;
  GLuint fallback_tex;
  GLuint program;
  GLint angle_loc;
  GLint color_loc;
  GLint has_texture_loc;
  mat4 identity = GLM_MAT4_IDENTITY_INIT;
  const char *node_name;
  const char *geom_name;

  if (argc != 2) {
    fprintf(stderr, "usage: %s path/to/model\n", argv[0]);
    return 2;
  }

  sample_gl_register_image_loader();
  if (!sample_load_doc(&doc, argv[1]))
    return 1;

  memset(&mesh, 0, sizeof(mesh));
  scene = doc->scene ? doc->scene : doc->lib.scenes.first;
  if (!scene || !sample_gl_find_first_mesh(scene->node ? scene->node->chld : NULL,
                                           identity,
                                           0u,
                                           &mesh,
                                           &node_name,
                                           &geom_name)) {
    fprintf(stderr, "no triangle primitive found in active scene\n");
    ak_free(doc);
    return 1;
  }

  light_count = 0;
  sample_gl_collect_lights(doc,
                           scene->node ? scene->node->chld : NULL,
                           identity,
                           &mesh,
                           0u,
                           lights,
                           SAMPLE_GL_MAX_LIGHTS,
                           &light_count);

  printf("mesh: node=%s geometry=%s vertices=%zu indices=%zu material_rgba=(%.3g %.3g %.3g %.3g)\n",
         sample_or_unnamed(node_name),
         sample_or_unnamed(geom_name),
         mesh.vertex_count,
         mesh.index_count,
         mesh.base_color[0],
         mesh.base_color[1],
         mesh.base_color[2],
         mesh.base_color[3]);
  printf("lights: %zu%s\n",
         light_count,
         doc->lib.lights.count > light_count ? " (truncated for the sample shader)" : "");

  if (!sample_gl_init_window(&window, "AssetKit GL 03 Lighting")) {
    sample_gl_free_mesh(&mesh);
    ak_free(doc);
    return 1;
  }

  tex = mesh.has_uv ? sample_gl_upload_texture(mesh.base_color_image) : 0u;
  fallback_tex = tex ? 0u : sample_gl_create_white_texture();
  program = sample_gl_make_program("03_lighting", "lighting.vert", "lighting.frag");
  if (!program) {
    if (tex)
      glDeleteTextures(1, &tex);
    if (fallback_tex)
      glDeleteTextures(1, &fallback_tex);
    glfwDestroyWindow(window);
    glfwTerminate();
    sample_gl_free_mesh(&mesh);
    ak_free(doc);
    return 1;
  }

  glGenVertexArrays(1, &vao);
  glGenBuffers(1, &vbo);
  glGenBuffers(1, &ebo);
  glBindVertexArray(vao);

  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER,
               (GLsizeiptr)(mesh.vertex_count * sizeof(*mesh.vertices)),
               mesh.vertices,
               GL_STATIC_DRAW);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, (GLsizei)sizeof(SampleGLVertex), (void *)offsetof(SampleGLVertex, position));
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, (GLsizei)sizeof(SampleGLVertex), (void *)offsetof(SampleGLVertex, normal));
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, (GLsizei)sizeof(SampleGLVertex), (void *)offsetof(SampleGLVertex, uv));
  glEnableVertexAttribArray(2);

  if (mesh.index_count) {
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 (GLsizeiptr)(mesh.index_count * sizeof(*mesh.indices)),
                 mesh.indices,
                 GL_STATIC_DRAW);
  }

  glBindVertexArray(0);
  angle_loc = glGetUniformLocation(program, "u_angle");
  color_loc = glGetUniformLocation(program, "u_base_color");
  has_texture_loc = glGetUniformLocation(program, "u_has_base_texture");
  glUseProgram(program);
  glUniform1i(glGetUniformLocation(program, "u_base_texture"), 0);
  sample_set_light_uniforms(program, lights, light_count);

  glEnable(GL_DEPTH_TEST);
  printf("close the window to exit\n");
  while (!glfwWindowShouldClose(window)) {
    int width;
    int height;
    float angle;

    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);
    glClearColor(0.08f, 0.09f, 0.10f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    angle = (float)glfwGetTime() * 0.25f;
    glUseProgram(program);
    glUniform1f(angle_loc, angle);
    glUniform4fv(color_loc, 1, mesh.base_color);
    glUniform1i(has_texture_loc, tex != 0u);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex ? tex : fallback_tex);
    glBindVertexArray(vao);
    if (mesh.index_count)
      glDrawElements(GL_TRIANGLES, (GLsizei)mesh.index_count, GL_UNSIGNED_INT, (void *)0);
    else
      glDrawArrays(GL_TRIANGLES, 0, (GLsizei)mesh.vertex_count);

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  if (tex)
    glDeleteTextures(1, &tex);
  if (fallback_tex)
    glDeleteTextures(1, &fallback_tex);
  glDeleteProgram(program);
  glDeleteBuffers(1, &ebo);
  glDeleteBuffers(1, &vbo);
  glDeleteVertexArrays(1, &vao);
  glfwDestroyWindow(window);
  glfwTerminate();
  sample_gl_free_mesh(&mesh);
  ak_free(doc);
  return 0;
}
