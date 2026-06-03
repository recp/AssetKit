/*
 * Copyright (C) 2020 Recep Aslantas
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef tests_h
#define tests_h

#include "include/common.h"

/*
 * To register a test:
 *   1. use TEST_DECLARE() to forward declare test
 *   2. use TEST_ENTRY() to add test to list
 */

TEST_DECLARE(heap)
TEST_DECLARE(heap_multiple)
TEST_DECLARE(instance_attach_helpers)
TEST_DECLARE(node_instance_bbox_traversal)
TEST_DECLARE(node_instance_bbox_lazy_geometry)
TEST_DECLARE(node_instance_bbox_path_state)
TEST_DECLARE(node_instance_bbox_rotated_ref)
TEST_DECLARE(node_instance_camera_world_path)
TEST_DECLARE(scene_find_or_make_root_uses_instance_nodes)
TEST_DECLARE(dae_scene_roots_are_instance_nodes)
TEST_DECLARE(dae_instance_node_is_instance_node)
TEST_DECLARE(obj_scene_root_is_instance_node)
TEST_DECLARE(material_api)
TEST_DECLARE(material_dae_adapter)
TEST_DECLARE(material_gltf_alpha_modes)
TEST_DECLARE(material_obj_adapter)
TEST_DECLARE(dae_load_folder)
TEST_DECLARE(index_stats_corpus)
TEST_DECLARE(format_edge_cases)

/*****************************************************************************/

TEST_LIST {
  TEST_ENTRY(heap)
  TEST_ENTRY(heap_multiple)
  TEST_ENTRY(instance_attach_helpers)
  TEST_ENTRY(node_instance_bbox_traversal)
  TEST_ENTRY(node_instance_bbox_lazy_geometry)
  TEST_ENTRY(node_instance_bbox_path_state)
  TEST_ENTRY(node_instance_bbox_rotated_ref)
  TEST_ENTRY(node_instance_camera_world_path)
  TEST_ENTRY(scene_find_or_make_root_uses_instance_nodes)
  TEST_ENTRY(dae_scene_roots_are_instance_nodes)
  TEST_ENTRY(dae_instance_node_is_instance_node)
  TEST_ENTRY(obj_scene_root_is_instance_node)
  TEST_ENTRY(material_api)
  TEST_ENTRY(material_dae_adapter)
  TEST_ENTRY(material_gltf_alpha_modes)
  TEST_ENTRY(material_obj_adapter)
  TEST_ENTRY(dae_load_folder)
  TEST_ENTRY(index_stats_corpus)
  TEST_ENTRY(format_edge_cases)
};

#endif /* tests_h */
