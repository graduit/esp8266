<?php
// Get the post variables
$value = $_POST['value'];
$key = $_POST['key'];

$response = new stdClass();
$response->one = $key . "test";
$response->two = $value . "test";

// Return json
header('Content-type: application/json');
echo json_encode($response);

 ?>