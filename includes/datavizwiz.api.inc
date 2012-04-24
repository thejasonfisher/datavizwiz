<?php

function datavizwiz_api($nid, $operation) {
  $rows = _datavizwiz_get_data_array($nid);

  switch (strtolower($operation)) {
    case 'csv':
      _datavizwiz_return_csv($rows);
      break;
    case 'geojson':
      //echo 'test';
      $latloncols = _datavizwiz_get_latlon_cols($nid);
      _datavizwiz_return_geojson($rows, $latloncols);
      break;
    case 'json':
      _datavizwiz_return_json($rows);
      break;
    case 'xml':
      _datavizwiz_return_xml($rows);
      break;
    default:
      echo 'Invalid operation';
      break;
  }
}

function _datavizwiz_get_data_array($nid) {
  if (!is_numeric($nid)) {
    die;
  } // Check for numeric node ID, stop if not
  $node = node_load($nid);
  $datatable = $node->field_dvw_datatable[0]['value'];

  //All of our tables from the Importer module are prefixed with import_, if not here, stop processing
  if (substr($datatable, 0, 7) != 'import_') {
    die;
  }

  $result = db_query(sprintf('SELECT * FROM {%s}', $datatable));

  while ($row = db_fetch_array($result)) {
    $rows[] = $row;
  }

  return $rows;
}

function _datavizwiz_get_latlon_columns($nid) {
  
}

function _datavizwiz_return_csv($rows) {
  drupal_set_header("Content-type: text/csv");
  drupal_set_header("Cache-Control: no-store, no-cache");
  drupal_set_header('Content-Disposition: attachment; filename="data.csv"');
  
  $colnames = _datavizwiz_get_column_names($rows[0]);

  echo _datavizwiz_export_csv($rows, $colnames);
}

function _datavizwiz_return_geojson($rows, $latloncols) {
  drupal_set_header("Content-type: application/json");
  drupal_set_header("Cache-Control: no-store, no-cache");
  
  if (count($latloncols) != 2) {
    $return = "Latitude and longitude is not defined for this data set.";
  } else {
    $latcol = '';
    $loncol = '';
    
    foreach($latloncols as $col) {
      if ($col['flag'] == DVW_FLAG_LATITUDE) {
        $latcol = $col['field_name'];
      }
      if ($col['flag'] == DVW_FLAG_LONGITUDE) {
        $loncol = $col['field_name'];
      }
    }
    
    if ($latcol == '' || $loncol == '') {
      $return = "Latitude and longitude is not defined for this data set.";
    } else {
      foreach($rows as $row) {
        $feature = new stdClass();
        $feature->type = 'Feature';

        $geometry = new stdClass();
        $geometry->type = 'Point';
        $geometry->coordinates = array($row[$loncol], $row[$latcol]); // sprintf('[%s, %s]', $y, $x);
        $feature->geometry = $geometry;

        $feature->id = $row['ImporterID'];
        $properties = new stdClass();
        $properties->popupContent = 'Test';
        $feature->properties = $properties;

        $geojson[] = $feature;
      }

      $return = new stdClass();
      $return->type = 'FeatureCollection';
      $return->features = $geojson;
    }
  }
  
  echo json_encode($return);
  
}

function _datavizwiz_return_json($rows) {
  // Drupal function: drupal_json does not return valid JSON
  echo json_encode($rows);
}

function _datavizwiz_return_xml($rows) {
  $colnames = _datavizwiz_get_column_names($rows[0]);
  $xml = simplexml_load_string("<dataset></dataset>");

  foreach($rows as $row) {
    $data = $xml->addChild('data');
    foreach($colnames as $colname) {
      $element = $data->addChild($colname, $row[$colname]);
    }
  }
  
  
  
  drupal_set_header("Content-Type:text/xml");
  echo $xml->asXML();
  //print_r($xml);
}

function _datavizwiz_return_d3_json($rows) {
  return "d3_json";
}

function _datavizwiz_export_csv($data, $col_headers = array(), $return_string = false) {
  $stream = ($return_string) ? fopen('php://temp/maxmemory', 'w+') : fopen('php://output', 'w');

  if (!empty($col_headers)) {
    fputcsv($stream, $col_headers);
  }

  foreach ($data as $record) {
    fputcsv($stream, $record);
  }

  if ($return_string) {
    rewind($stream);
    $retVal = stream_get_contents($stream);
    fclose($stream);
    return $retVal;
  }
  else {
    fclose($stream);
  }
}

function _datavizwiz_get_column_names($array) {
  foreach(array_keys($array) as $colname) {
    $colnames[] = $colname;
  }
  
  return $colnames;
}