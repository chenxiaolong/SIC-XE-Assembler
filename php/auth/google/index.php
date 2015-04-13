<?php

require_once __DIR__.'/../../vendor/autoload.php';
require_once __DIR__.'/../../config.php';
require_once __DIR__.'/../auth.inc.php';


function fix_profile_image_url($url) {
    // Remove "sz=<size>" query parameter so don't get a tiny 50px image
    $parsed = parse_url($url);
    parse_str($parsed['query'], $query);
    if (isset($query['sz'])) {
        unset($query['sz']);
    }
    $querystr = http_build_query($query);
    if (empty($querystr)) {
        unset($parsed['query']);
    } else {
        $parsed['query'] = $querystr;
    }
    return http_build_url($parsed);
}


$provider = new League\OAuth2\Client\Provider\Google([
    'clientId'      => GOOGLE_CLIENT_ID,
    'clientSecret'  => GOOGLE_CLIENT_SECRET,
    'redirectUri'   => GOOGLE_REDIRECT_URI,
    'scopes'        => ["https://www.googleapis.com/auth/userinfo.email"]
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

        $username = $user_data->uid;
        $name = $user_data->name;
        $email = $user_data->email;
        $image = fix_profile_image_url($user_data->imageUrl);

        // Standard login
        auth_login(array(
            'provider' => 'google',
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
