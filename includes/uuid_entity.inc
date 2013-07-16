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

function uuid_entity_features_export_options()
{
    $options = array();
    $entity_info = entity_get_info();
    foreach ($entity_info as $entity_type => $type_info)
    {
        if (!isset($type_info['uuid'])) {continue;}
        $entities = entity_load($entity_type, FALSE);
        foreach ($entities as $entity)
        {
            $options[$entity->uuid] = t('@type @bundle: @title', array(
                '@type' => $entity_type,
                '@bundle' => !empty($type_info['entity keys']['bundle']) ? '- ' . $entity->{$type_info['entity keys']['bundle']} : '',
                '@title' => entity_label($entity_type, $entity),
            ));
        }
    }
    return $options;
}

/**
 * Implements hook_features_export().
 */

function uuid_entity_features_export($data, &$export, $module_name = '')
{
    $pipe = array();

    $export['dependencies']['uuid_features'] = 'uuid_features';

    uuid_features_load_module_includes();
    $map = features_get_component_map('uuid_entity');
    $entity_info = entity_get_info();
    foreach($entity_info as $entity_type => $type_info)
    {

    }
}