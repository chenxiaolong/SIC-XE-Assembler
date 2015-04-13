<?php

require_once __DIR__.'/../../vendor/autoload.php';
require_once __DIR__.'/../../config.php';
require_once __DIR__.'/../auth.inc.php';


function file_get_contents_ua($url) {
    $options = array('http' => array(
        'user_agent' => 'SIC/XE Assembler'
    ));
    $context = stream_context_create($options);
    return file_get_contents($url, false, $context);
}

function get_avatar_url($username) {
    $url = 'https://api.github.com/users/'.$username;
    $jstr = file_get_contents_ua($url);
    $json = json_decode($jstr, true);
    return $json['avatar_url'];
}


$provider = new League\OAuth2\Client\Provider\Github([
    'clientId'      => GITHUB_CLIENT_ID,
    'clientSecret'  => GITHUB_CLIENT_SECRET,
    'redirectUri'   => GITHUB_REDIRECT_URI,
    'scopes'        => []
]);

# Initialize session
if (session_status() == PHP_SESSION_NONE) {
    session_start();
}

# Ensure we're called by who we expect
if (!auth_check_state()) {
    redirect('../..');
    exit();
}

# Handle callback
if (isset($_GET['code'])) {
    // Try to get the access token
    $token = $provider->getAccessToken('authorization_code', [
        'code' => $_GET['code']
    ]);

    // Try to get user details
    try {
        $user_data = $provider->getUserDetails($token);

        $username = $user_data->nickname;
        $name = $user_data->name;
        $email = $user_data->email;
        $image = get_avatar_url($username);

        // Standard login
        auth_login(array(
            'provider' => 'github',
            'username' => $username,
            'name' => $name,
            'email' => $email,
            'image' => $image
        ));
    } catch (Exception $e) {
        error_log($e);
        redirect('../..');
        exit();
    }

    // Redirect back to index
    redirect('../..');

    exit();
}

if (!auth_valid_action()) {
    redirect('../..');
    exit();
}

$action = auth_get_action();

if ($action == AUTH_ACTION_LOGOUT) {
    // Logout
    auth_logout();

    // Redirect back to index
    redirect('../..');
} elseif ($action == AUTH_ACTION_LOGIN) {
    $auth_url = $provider->getAuthorizationUrl();
    auth_set_session_state($provider->state);
    redirect($auth_url);
}

?>
