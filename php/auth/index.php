<?php

require_once 'auth.inc.php';

function redirect_to_main() {
    redirect('..');
}

// Initialize session
if (session_status() == PHP_SESSION_NONE) {
    session_start();
}

// If we're missing GET parameters, go back to main
if (!isset($_GET['action']) ||
        ($_GET['action'] == 'login' && !isset($_GET['provider']))) {
    redirect_to_main();
    exit();
}

$action = $_GET['action'];

if ($action == 'login') {
    $provider = $_GET['provider'];

    if (!auth_is_valid_provider($provider)) {
        redirect_to_main();
        exit();
    }

    // If we're already authenticated, go back to main
    if (auth_is_authenticated()) {
        redirect_to_main();
        exit();
    }

    $state = md5(rand());
    auth_set_session_state($state);

    redirect($provider.'/'.
             '?action='.AUTH_ACTION_LOGIN.
             '&state='.$state);
} elseif ($action == 'logout') {
    $provider = auth_get_provider();

    if (!auth_is_valid_provider($provider)) {
        redirect_to_main();
        exit();
    }

    // If we're already unauthenticated, go back to main
    if (!auth_is_authenticated()) {
        redirect_to_main();
        exit();
    }

    $state = md5(rand());
    auth_set_session_state($state);

    redirect($provider.'/'.
             '?action='.AUTH_ACTION_LOGOUT.
             '&state='.$state);
} else {
    redirect_to_main();
}

?>
