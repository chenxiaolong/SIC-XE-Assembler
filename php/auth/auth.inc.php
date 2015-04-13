<?php

const AUTH_ACTION_LOGIN = 'login';
const AUTH_ACTION_LOGOUT = 'logout';

function redirect($url) {
    header('Location: '.filter_var($url, FILTER_SANITIZE_URL));
}

function auth_is_valid_provider($provider) {
    return in_array($provider, array(
        'github',
        //'bitbucket',
        'google'
    ));
}

function auth_set_session_state($state) {
    $_SESSION['state'] = $state;
}

function auth_check_state() {
    if (!isset($_GET['state']) || !isset($_SESSION['state'])) {
        return false;
    }

    $state = $_SESSION['state'];
    unset($_SESSION['state']);

    return $_GET['state'] == $state;
}

function auth_get_action() {
    return $_GET['action'];
}

function auth_valid_action() {
    if (!isset($_GET['action'])) {
        return false;
    }

    return in_array($_GET['action'], array(
        AUTH_ACTION_LOGIN,
        AUTH_ACTION_LOGOUT
    ));
}

function auth_logout() {
    unset($_SESSION['provider']);
    unset($_SESSION['username']);
    unset($_SESSION['name']);
    unset($_SESSION['email']);
    unset($_SESSION['image']);
    unset($_SESSION['authenticated']);
}

function auth_login($params) {
    if (empty($params['provider']) ||
            empty($params['username']) ||
            empty($params['name']) ||
            empty($params['email'])) {
        error_log('Incomplete login details');
        return false;
    }

    error_log('User logged in: '.
              'provider="'.$params['provider'].'", '.
              'username="'.$params['username'].'", '.
              'name="'.$params['name'].'", '.
              'email='.$params['email'].'"');

    $_SESSION['provider'] = $params['provider'];
    $_SESSION['username'] = $params['username'];
    $_SESSION['name'] = $params['name'];
    $_SESSION['email'] = $params['email'];

    // Optional
    if (isset($params['image'])) {
        $_SESSION['image'] = $params['image'];
    }

    $_SESSION['authenticated'] = true;

    return true;
}

function auth_is_authenticated() {
    return isset($_SESSION['authenticated']) &&
            $_SESSION['authenticated'];
}

function auth_get_provider() {
    if (!isset($_SESSION['provider'])) {
        return null;
    }
    return $_SESSION['provider'];
}

function auth_get_username() {
    if (!isset($_SESSION['username'])) {
        return null;
    }
    return $_SESSION['username'];
}

function auth_get_name() {
    if (!isset($_SESSION['name'])) {
        return null;
    }
    return $_SESSION['name'];
}

function auth_get_email() {
    if (!isset($_SESSION['email'])) {
        return null;
    }
    return $_SESSION['email'];
}

function auth_get_image() {
    if (!isset($_SESSION['image'])) {
        return null;
    }
    return $_SESSION['image'];
}

?>
