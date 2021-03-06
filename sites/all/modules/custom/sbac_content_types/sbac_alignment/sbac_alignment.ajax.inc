<?php

/**
 * Page Callback
 * Outputs the alignment tagger UI.
 */
function sbac_alignment_ajax_alignment_page($ref_node = NULL) {
  drupal_add_js(libraries_get_path('jquery.more') . '/jquery.more.js');
  drupal_add_css(libraries_get_path('jquery.more') . '/jquery.more.css');
  drupal_add_js(drupal_get_path('theme', 'sbac') . '/js/custom.js');
  drupal_add_css(drupal_get_path('module', 'sbac_alignment') . '/css/alignment.css');

  $vocab = taxonomy_vocabulary_machine_name_load(SBAC_ALIGNMENT_VOCAB_NAME);
  $terms = taxonomy_get_tree($vocab->vid, 0, SBAC_ALIGNMENT_MAX_DEPTH);

  $output = '<div id="alignment-page"><input id="ref_node" style="display:none" value="' . $ref_node . '"> ';
  $output .= '<div class="alignment-breadcrumb"></div>';
  $output .= '<div id="alignment-msg"></div>';
  $output .= '<div class="alignment-filter">';
  $output .= theme('alignment_tags', array('terms' => $terms, 'level_title' => 'Choose Your Subject Area'));
  $output .= '</div><div class="alignment-form"></div>';
  $output .= '<div class="alignment-buttons"><a href="#" title="Add Standards" id="edit-submit" class="button disabled">Add Standard(s) to Resource</a><a href="#" title="Cancel" id="ccss-cancel" class="button ctools-close-modal close">Cancel</a></div>';
  $output .= '</div></div>';

  return $output;
}

/**
 * Page Callback
 * Outputs the alignment tagger UI.
 */
function sbac_alignment_ajax_filter_alignment_page($ref_node = NULL) {
  drupal_add_js(libraries_get_path('jquery.more') . '/jquery.more.js');
  drupal_add_css(libraries_get_path('jquery.more') . '/jquery.more.css');
  drupal_add_js(drupal_get_path('theme', 'sbac') . '/js/custom.js');
  drupal_add_css(drupal_get_path('module', 'sbac_alignment') . '/css/alignment.css');

  $vocab = taxonomy_vocabulary_machine_name_load(SBAC_ALIGNMENT_VOCAB_NAME);
  $terms = taxonomy_get_tree($vocab->vid, 0, SBAC_ALIGNMENT_MAX_DEPTH);

  $output = '<div id="alignment-page"><input id="ref_node" style="display:none" value="' . $ref_node . '"> ';
  $output .= '<div class="alignment-breadcrumb"></div>';
  $output .= '<div id="alignment-msg"></div>';
  $output .= '<div class="alignment-filter">';
  $output .= theme('alignment_tags', array('terms' => $terms, 'level_title' => 'Choose Your Subject Area'));
  $output .= '</div><div class="alignment-form"></div>';
  $output .= '<div class="alignment-buttons"><a href="#" title="Add Selections" id="edit-submit" class="button disabled">Add Selections</a>';
  $output .= '<a href="#" title="Cancel" id="ccss-cancel" class="button ctools-close-modal close">Cancel</a></div>';
  $output .= '</div></div>';

  return $output;
}


/**
 * AJAX Callback
 */
function sbac_alignment_ajax_terms_callback() {
  $subject_array = $output_array = array();
  $ela_titles = array(
    'Choose Your Subject Area',
    'Choose Your Grade',
    'Choose Your Strand/Domain',
    'Choose Your Common Core Standard(s), ELA Anchor Standard(s), and/or Mathematical Practice(s)',
  );
  $ma_titles = array(
    'Choose Your Subject Area',
    'Choose Your Grade',
    'Choose Your Domain',
    'Choose Your Common Core Standard(s), ELA Anchor Standard(s), and/or Mathematical Practice(s)',
  );

  $vocab = taxonomy_vocabulary_machine_name_load(SBAC_ALIGNMENT_VOCAB_NAME);
  $parent = taxonomy_term_load($_POST['parent']);
  if (isset($_POST['remove'])) {
    $temp = array_shift(taxonomy_get_parents($parent->tid));
    if (empty($temp)) {
      $parent->tid = 0;
    }
    else {
      $parent = $temp;
    }
  }

  $output_array['parent'] = $parent;
  $terms = taxonomy_get_tree($vocab->vid, $parent->tid, SBAC_ALIGNMENT_MAX_DEPTH);
  $publication = $parent->field_alignment_publication['und'][0]['value'];
  $output_array['publication'] = (empty($publication)) ? 0 : $publication;
  $output_array['depth'] = $depth = sbac_alignment_term_depth($parent->tid);
  if ($publication == 'CC-MA-v1') {
    $level_title = $ma_titles[$depth];
  }
  else {
    $level_title = $ela_titles[$depth];
  }

  $output_array['tids'] = array();
  foreach ($terms as $term) {
    $pos = strpos($term->name, ':');
    if ($pos !== FALSE) {
      $new_name = substr($term->name, $pos + 1);
      $term->name = trim($new_name);
    }

    $output_array['tids'][] = $term;
    $children = taxonomy_get_children($term->tid);
    $output_array['children'][] = $children;

    $options = array(
      'attributes' => array(
        'id' => 'term-' . $term->tid,
        'class' => 'sbac-custom-term button',
        'tid' => $term->tid,
        'title' => $term->name,
      ),
      'html' => TRUE,
    );
    $subject_array[] = l($term->name, '#', $options);
  }

  $output_array['tids_count'] = count($output_array['tids']);
  $output = theme('alignment_tags', array('terms' => $terms, 'level_title' => $level_title));
  $output_array['html'] = $output;

  print drupal_json_encode($output_array);
  exit;
}

function sbac_alignment_ajax_filter_alignment_confirm_remove_callback() {
  $output = theme('alignment_modal_confirm', array('terms' => array()));
  $output_array['html'] = $output;
  print drupal_json_encode($output_array);
  exit;
}

/**
 * AJAX Callback
 * CRUD for Alignment nodes.
 */
function sbac_alignment_ajax_alignment_crud_callback() {
  global $user;
  $op = 'none';
  $nid = 0;
  if (isset($_POST['op'])) {
    $op = $_POST['op'];
  }

  switch ($op) {
    case 'delete':
      if (isset($_POST['nid'])) {
        $nid = $_POST['nid'];
      }
      // prep JSON response
      $data = array(
        'deleted_nid' => $nid,
      );

      $node = node_load($nid);
      if (node_access('delete', $node)) {
        // node_delete($nid);
        $current_nid = $_POST['current_nid'];
        // set the entity reference field_data_field_resource_reference deleted flag to 2
        db_query('UPDATE field_data_field_resource_reference SET deleted = :deleted WHERE field_resource_reference_target_id = :target_id AND entity_id = :entity_id', array(':deleted' => 2, ':target_id' => $current_nid, ':entity_id' => $nid));
      }

      print drupal_json_encode($data);

      break;

    case 'create':
      if (isset($_POST['alignment_standards'])) {
        $standards = explode('|', $_POST['alignment_standards']);
        array_shift($standards);
      }
      if (isset($_POST['alignment_type'])) {
        $alignment_type = $_POST['alignment_type'];
      }
      if (isset($_POST['alignment_ref'])) {
        $alignment_ref = $_POST['alignment_ref'];
      }
      else {
        $alignment_ref = 537;
      }

      $types = array();
      $vocab = taxonomy_vocabulary_machine_name_load(SBAC_ALIGNMENT_TYPE_VOCAB_NAME);
      $terms = taxonomy_get_tree($vocab->vid, 0, SBAC_ALIGNMENT_MAX_DEPTH);

      foreach ($terms as $term) {
        $types[$term->name] = $term->tid;
      }

      $standards_terms = array();
      foreach ($standards as $tid) {
        //create alignment
        $node = new stdClass();
        $node->title = "resource-$alignment_ref-alignment-$tid";
        $node->type = 'alignment';
        $node->uid = $user->uid;
        node_object_prepare($node);

        $node->field_education_alignment['und'][]['tid'] = $tid;
        $node->field_alignment_type['und'][]['tid'] = $types[$alignment_type];
        $node->field_resource_reference['und'][]['target_id'] = $alignment_ref;
        node_save($node);

        // Set the deleted flag to 1 to create a temp new alignment.
        db_query('UPDATE field_data_field_resource_reference SET deleted = :deleted WHERE field_resource_reference_target_id = :target_id AND entity_id = :entity_id', array(':deleted' => 1, ':target_id' => $alignment_ref, ':entity_id' => $node->nid));

        $term = taxonomy_term_load($tid);
        $standards_terms[] = $term->name;
      }

      $data = array(
        'html' => views_embed_view('alignment', 'resource_admin_view', $alignment_ref),
        'terms' => $standards_terms,
      );

      print drupal_json_encode($data);

      exit;
      break;
  }
  exit;
}

/**
 * AJAX Callback
 * CRUD for Alignment nodes.
 */
function sbac_alignment_ajax_filter_alignment_finish_set_callback() {
  global $user;
  $op = 'none';
  if (isset($_POST['op'])) {
    $op = $_POST['op'];
  }

  if (isset($_POST['alignment_standards'])) {
    $standards = explode('|', $_POST['alignment_standards']);
    array_shift($standards);
  }

  $types = array();
  $vocab = taxonomy_vocabulary_machine_name_load(SBAC_ALIGNMENT_VOCAB_NAME);

  $standards_terms = array();
  $filter_markup = array();
  $filter_hidden = array();

  $term_information = taxonomy_term_load_multiple($standards);
  foreach ($standards as $tid) {
    $label = '';
    $term_wrapper = entity_metadata_wrapper('taxonomy_term', $term_information[$tid]);
    if (isset($term_wrapper->field_alignment_shortname)) {
      $label = $term_wrapper->field_alignment_shortname->value();
    } else {
      $label = $term_wrapper->name->value();
    }
    $filter_text = '<div class="current-filter" class="filter-name" tid="'.$tid.'" vid="'.$vocab->vid.'">';
    $filter_text .= $label;
    $filter_text .= '</div>';
    $filter_markup[] = $filter_text;

    $filter_hidden[] = $vocab->vid . ':' . $tid;
  }

  $html = implode('',$filter_markup);
  $filters = implode('::',$filter_hidden);
  $data = array(
    'html' => $html,
    'filters' => $filters,
  );

  print drupal_json_encode($data);
  exit;
}

/**
 * AJAX Callback
 * Lazy-loading the form.
 */
function sbac_alignment_ajax_alignment_form_callback() {
  $tid = $_POST['tid'];
  $output['html'] = drupal_render(drupal_get_form('sbac_alignment_alignment_form', $tid));
  print drupal_json_encode($output);
  exit;
}
/**
 * AJAX Callback - filter dialog for ccss
 * Lazy-loading the form.
 */
function sbac_alignment_ajax_filter_alignment_form_callback() {
  $tid = $_POST['tid'];
  $output['html'] = drupal_render(drupal_get_form('sbac_alignment_filter_alignment_form', $tid));
  print drupal_json_encode($output);
  exit;
}


/**
 * AJAX Callback
 * Outputs the breadcumbs.
 */
function sbac_alignment_ajax_alignment_breadcrumbs_callback() {
  $output = $terms = array();
  $parents = taxonomy_get_parents_all($_POST['tid']);

  foreach ($parents AS $key => $term) {
    $pos = strpos($term->name, ':');
    // Remove the ":" from the string.
    if ($pos !== FALSE) {
      $new_name = substr($term->name, $pos + 1);
      $term->name = trim($new_name);
      $parents[$key] = $term;
    }

    // Trim the name if its greater then 41 characters.
    if (strlen($term->name) > 41) {
      $term->name = substr($term->name, 0, 41) . '...';
      $parents[$key] = $term;
    }
  }

  $output['html'] = theme('alignment_breadcrumbs', array('terms' => array_reverse($parents)));
  print drupal_json_encode($output);
  exit;
}
