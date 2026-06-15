/*
 * OpenGL sample 01: draw the first scene mesh.
 *
 * This is intentionally minimal: load, find a triangle primitive, ask
 * AssetKit for a single-index view, upload a vertex/index buffer, draw.
 */

#include "../gl_sample_common.h"

int
main(int argc, char **argv) {
  AkDoc *doc;
  AkScene *scene;
  SampleGLMesh mesh;
  GLFWwindow *window;
  GLuint vao;
  GLuint vbo;
  GLuint ebo;
  GLuint program;
  GLint angle_loc;
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

  printf("mesh: node=%s geometry=%s vertices=%zu indices=%zu\n",
         sample_or_unnamed(node_name),
         sample_or_unnamed(geom_name),
         mesh.vertex_count,
         mesh.index_count);

  if (!sample_gl_init_window(&window, "AssetKit GL 01 Mesh")) {
    sample_gl_free_mesh(&mesh);
    ak_free(doc);
    return 1;
  }

  program = sample_gl_make_program("01_mesh", "mesh.vert", "mesh.frag");
  if (!program) {
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
  glVertexAttribPointer(0,
                        3,
                        GL_FLOAT,
                        GL_FALSE,
                        (GLsizei)sizeof(SampleGLVertex),
                        (void *)offsetof(SampleGLVertex, position));
  glEnableVertexAttribArray(0);

  if (mesh.index_count) {
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 (GLsizeiptr)(mesh.index_count * sizeof(*mesh.indices)),
                 mesh.indices,
                 GL_STATIC_DRAW);
  }

  glBindVertexArray(0);
  angle_loc = glGetUniformLocation(program, "u_angle");

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

    angle = (float)glfwGetTime() * 0.45f;
    glUseProgram(program);
    glUniform1f(angle_loc, angle);
    glBindVertexArray(vao);
    if (mesh.index_count)
      glDrawElements(GL_TRIANGLES, (GLsizei)mesh.index_count, GL_UNSIGNED_INT, (void *)0);
    else
      glDrawArrays(GL_TRIANGLES, 0, (GLsizei)mesh.vertex_count);

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

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
