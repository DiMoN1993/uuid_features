<?php
/**
 * Created by JetBrains PhpStorm.
 * User: developer
 * Date: 7/10/13
 * Time: 4:25 PM
 * To change this template use File | Settings | File Templates.
 */


/**
 * Implements hook_features_export_options().
 */
$entity_info = entity_get_info();
foreach ($entity_info as $entity_type => $type_info) {
  if (!isset($type_info['uuid'])) {
    continue;
  }
  if (!function_exists("uuid_{$entity_type}_features_export_options")) {
    $function_code = "function uuid_{$entity_type}_features_export_options() {return uuid_entity_features_export_options(\"$entity_type\");  }";
    eval($function_code);
  }
  if (!function_exists("uuid_{$entity_type}_features_export")) {
    $function_code = "function uuid_{$entity_type}_features_export(\$data, &\$export, \$module_name = '') {return uuid_entity_features_export(\"$entity_type\", \$data, \$export, \$module_name);  }";
    eval($function_code);
  }
  if (!function_exists("uuid_{$entity_type}_features_export_render")) {
    $function_code = "function uuid_{$entity_type}_features_export_render(\$module, \$data, \$module_name = '') {return uuid_entity_features_export_render(\"$entity_type\", \$module, \$data, \$module_name);  }";
    eval($function_code);
  }
  if (!function_exists("uuid_{$entity_type}_features_rebuild")) {
    $function_code = "function uuid_{$entity_type}_features_rebuild(\$module) {return uuid_entity_features_rebuild(\"$entity_type\", \$module);  }";
    eval($function_code);
  }
}

function uuid_entity_features_export_options($entity_type) {
  $options = array();
  $type_info = entity_get_info($entity_type);
  $entities = entity_load($entity_type, FALSE);
  foreach ($entities as $entity) {
    $options[$entity->uuid] = t('@type @bundle: @title', array(
      '@type' => $entity_type,
      '@bundle' => !empty($type_info['entity keys']['bundle']) ? '- ' . $entity->{$type_info['entity keys']['bundle']} : '',
      '@title' => entity_label($entity_type, $entity),
    ));
  }
  return $options;
}


/**
 * Implements hook_features_export().
 */
function uuid_entity_features_export($entity_type, $data, &$export, $module_name = '') {
  $pipe = array();
  $container = array();
  $dependences = array();
  $export['dependencies']['uuid_features'] = 'uuid_features';
  $ids = entity_get_id_by_uuid($entity_type, $data);
  foreach ($ids as $id) {
    $container["{$entity_type}_{$id}"] = array('type' => $entity_type, 'id' => $id);
  }

  while (!empty($container)) {
    $container_pop = array_pop($container);
    $pop_entity_type = $container_pop['type'];
    $pop_entity_id = $container_pop['id'];
    $entitys = entity_load($pop_entity_type, array($pop_entity_id));
    $entity = reset($entitys);
    $deps = module_invoke_all('entity_dependencies', $entity, $pop_entity_type);
    if ($entity_type == 'taxonomy_term') {
      $vocabulary = taxonomy_vocabulary_load($entity->vid);
      $export['features']['taxonomy'][$vocabulary->machine_name] = $vocabulary->machine_name;
      $uuids = entity_get_uuid_by_id($pop_entity_type, array($pop_entity_id));
      $uuid = current($uuids);
      $export['features']["uuid_{$pop_entity_type}"][$uuid] = $uuid;
    }
    else {
      $uuids = entity_get_uuid_by_id($pop_entity_type, array($pop_entity_id));
      $uuid = current($uuids);
      $export['features']["uuid_{$pop_entity_type}"][$uuid] = $uuid;
    }

    foreach ($deps as $piece) {
      $type = $piece['type'];
      $id = $piece['id'];
      $key = "{$type}_{$id}";
      if (!empty($dependences[$key])) {
        continue;
      }
      $container[$key] = $piece;
      $dependences[$key] = $piece;
    }
  }
  return $pipe;
}

/**
 * Implements hook_features_export_render().
 */
function uuid_entity_features_export_render($entity_type, $module, $data) {
  $code = array();

  uuid_features_load_module_includes();

  $code[] = "  \${$entity_type}s = array();";
  $code[] = '';
  $ids = entity_get_id_by_uuid($entity_type, $data);
  $entities = entity_uuid_load($entity_type, array_keys($ids), NULL, TRUE);
  foreach ($ids as $uuid => $id) {
    // Only export the node if it exists.
    if ($id === FALSE || !isset($entities[$id])) {
      continue;
    }
    // Attempt to load the node, using a fresh cache.

    $entity = $entities[$id];
    $export = clone $entity;
    $black_list = module_invoke_all('uuid_features_entity_render_blacklist');
    if (isset($black_list[$entity_type])) {
      foreach ($black_list[$entity_type] as $list) {
        unset($export->$list);
      }
    }

    drupal_alter("uuid_{$entity_type}_features_export_render", $export, $entity, $module);

    $data = array(
      'module' => $module,
      'bundle' => $entity_type,
      'type' => entity_label($entity_type,$entity)
    );
    if ($entity_type == 'file') {
      $data['type'] = $entity_type;
      $real_path = drupal_realpath($export->uri);
      $export->data = file_exists($real_path) ? base64_encode(file_get_contents($real_path)) : '';
    }
    elseif ($entity_type == 'taxonomy_vocabulary') {
      continue; //a temporary stop-gap. later me change that.
    }
    elseif ($entity_type == 'user' && isset($export->picture)) {
      $fid = entity_get_uuid_by_id('file', array($export->picture->fid));
      $export->picture = new stdClass();
      $export->picture->fid = current($fid);
    }
    $code[] = "  \${$entity_type}s[] = " . features_var_export($export) . ';';
  }

  $code[] = "  return \${$entity_type}s;";
  $code = implode("\n", $code);
  return array("uuid_features_default_$entity_type" => $code);
}

/**
 * Implements hook_features_rebuild().
 * Rebuilds entities based on UUID from code defaults.
 */
function uuid_entity_features_rebuild($entity_type, $module) {
  $entities = module_invoke($module, "uuid_features_default_$entity_type");
  if (!empty($entities)) {
    module_load_include('inc', $entity_type, "$entity_type.pages");
    foreach ($entities as $data) {
      $entity = (object) $data;
      if ((0 != $entity->uid) && (1 != $entity->uid)) {
        $id = entity_get_id_by_uuid('user', array($entity->uid));
        $id = current($id);
      }
      else {
        $id = $entity->uid;
      }
      entity_object_load($id, $entity_type);
      $entity->uid = (int) $id;
      // Find the matching UUID, with a fresh cache.
      $ids = entity_get_id_by_uuid($entity_type, array($entity->uuid));
      $id = array_key_exists($entity->uuid, $ids) ? $ids[$entity->uuid] : FALSE;
      if (!empty($id)) {
        $existing = entity_load($id, NULL, TRUE);
        if (!empty($existing)) {
          $entity->nid = $existing->nid;
          $entity->vid = $existing->vid;
        }
        else {
          $entity->is_new = TRUE;
        }
      }
      drupal_alter('uuid_entity_features_rebuild', $entity, $module);
      $entity = new entity($entity);
      entity_uuid_save($entity_type, $entity);
    }
  }
}
/**
 * Implements hook_uuid_features_entity_render_blacklist()
 */
function uuid_features_uuid_features_entity_render_blacklist() {
  return array(
    'node' => array('nid', 'vid', 'revision_uid', 'revision_timestamp'),
    'file' => array('uid', 'fid', 'timestamp', 'filesize'),
    'user' => array('uid', 'created', 'login', 'access', 'data'),
    'taxonomy_term' => array('vid', 'tid', 'vocabulary_machine_name')
  );
}