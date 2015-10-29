<?php

require_once 'auth/auth.inc.php';
require_once 'config.php';

const DISPLAY_TYPE_LOGIN            = 1;
const DISPLAY_TYPE_INPUT            = 2;
const DISPLAY_TYPE_RESULT           = 3;
const DISPLAY_TYPE_ERROR            = 4;

function process_post_request() {
    global $asm_contents, $lst_contents, $obj_contents, $error_msg;

    # Check that the required variables are set
    if (!isset($_POST['assembly'])) {
        # Kill bad requests
        exit();
    }

    $asm_contents = $_POST['assembly'];

    # Write the input to a temporary file
    $temp = tmpfile();
    $meta_data = stream_get_meta_data($temp);
    $asm_file = $meta_data['uri'];
    fwrite($temp, $asm_contents);

    # Generated files
    $lst_file = $asm_file.'.lst';
    $obj_file = $asm_file.'.obj';

    # Assemble!
    exec(SICASM_PATH.' '
            .escapeshellarg($asm_file).' 2>&1', $output, $exit_code);

    if ($exit_code != 0) {
        $error_msg = implode("\n", $output);
        $ret = false;
    } else {
        # Output
        $lst_contents = file_get_contents($lst_file);
        $obj_contents = file_get_contents($obj_file);
        $ret = true;
    }

    # Close file handles and delete temporary files
    fclose($temp);
    if (file_exists($lst_file)) {
        unlink($lst_file);
    }
    if (file_exists($obj_file)) {
        unlink($obj_file);
    }

    return $ret;
}

# Initialize session
if (session_status() == PHP_SESSION_NONE) {
    session_start();
}

if (!auth_is_authenticated()) {
    $display_type = DISPLAY_TYPE_LOGIN;
} elseif ($_SERVER['REQUEST_METHOD'] == 'POST') {
    if (process_post_request()) {
        $display_type = DISPLAY_TYPE_RESULT;
    } else {
        $display_type = DISPLAY_TYPE_ERROR;
    }
} else {
    $display_type = DISPLAY_TYPE_INPUT;
}

?>

<!DOCTYPE html>
<!--[if lt IE 7]><html class="no-js lt-ie9 lt-ie8 lt-ie7"><![endif]-->
<!--[if IE 7]><html class="no-js lt-ie9 lt-ie8"><![endif]-->
<!--[if IE 8]><html class="no-js lt-ie9"><![endif]-->
<!--[if gt IE 8]><!--><html class="no-js"><!--<![endif]-->
    <head>
        <meta charset="utf-8">
        <meta http-equiv="X-UA-Compatible" content="IE=edge,chrome=1">
        <meta name="viewport" content="width=device-width, initial-scale=1">
        <title>SIC/XE Assembler</title>

        <!-- Bootstrap CSS -->
        <link rel="stylesheet" href="bower_components/bootstrap/dist/css/bootstrap.min.css">

        <!-- Material Design CSS -->
        <link rel="stylesheet" href="bower_components/bootstrap-material-design/dist/css/material-fullpalette.min.css">
        <link rel="stylesheet" href="bower_components/bootstrap-material-design/dist/css/ripples.min.css">
        <link rel="stylesheet" href="bower_components/bootstrap-material-design/dist/css/roboto.min.css">

        <!-- Font-Awesome CSS -->
        <link rel="stylesheet" href="bower_components/font-awesome/css/font-awesome.min.css">

        <!-- Bootstrap-Social CSS -->
        <link rel="stylesheet" href="bower_components/bootstrap-social/bootstrap-social.css">

        <style>
            textarea {
                font-family: monospace;
                resize: none;
                overflow: hidden;
            }

            pre {
                background-color: inherit;
                border-style: none;
                overflow-x: auto;
            }

            pre code {
                overflow-wrap: normal;
                word-wrap: normal;
                white-space: pre;
                font-family: monospace;
                display: block;
            }

            .avatar-image-nav {
                position: relative;
                float: right;
                top: -5px;
                height: 30px;
                margin-left: 5px;
            }

            .avatar-icon-nav {
                position: relative;
                float: right;
                top: -5px;
                font-size: 30px;
                margin-left: 5px;
            }

            .avatar-image {
                padding-left: 15px;
                padding-right: 15px;
                padding-top: 5px;
                padding-bottom: 5px;
                height: 192px;
            }

            .avatar-icon {
                padding-left: 15px;
                padding-right: 15px;
                padding-top: 5px;
                padding-bottom: 5px;
                font-size: 192px;
                display: block;
                text-align: center;
                color: #808080;
            }
        </style>

        <!-- Line numbering -->
        <style>
            pre .line-number {
                font-family: monospace;
                float: left;
                margin-right: 1em;
                border-right: 1px solid;
                text-align: right;
            }

            pre .line-number span {
                display: block;
                padding-right: 1em;
            }
        </style>

        <!-- Main CSS -->
        <style>
            body {
                padding-top: 80px;
                padding-bottom: 20px;
                font-family: RobotoDraft, Roboto, Helvetica Neue, Helvetica, Arial, sans-serif;
            }
            iframe, img, embed {
                border: none;
            }
            .mdi {
                position: relative;
                top: 1px;
            }

            /* Small Devices */
            @media (max-width: 1200px) {
                body {
                    padding-top: 70px;
                }
            }

            /* Extra-Small Devices */
            @media (max-width: 767px) {
                .table {
                    table-layout: fixed;
                    overflow: hidden;
                }
            }
        </style>
    </head>

    <body>
        <nav class="navbar navbar-default navbar-fixed-top navbar-material-indigo-600">
            <div class="container">
                <div class="navbar-header">
                    <button type="button" class="navbar-toggle" data-toggle="collapse" data-target=".navbar-responsive-collapse">
                        <span class="icon-bar"></span>
                        <span class="icon-bar"></span>
                        <span class="icon-bar"></span>
                    </button>
                    <a class="navbar-brand" href="./">SIC/XE Assembler</a>
                </div>

                <div class="navbar-collapse collapse navbar-responsive-collapse">
                    <ul class="nav navbar-nav">
                        <li class="active"><a href="./">Home</a></li>
                        <li><a href="http://en.wikipedia.org/wiki/Simplified_Instructional_Computer" target="_blank">Wikipedia</a></li>
                    </ul>
<?php if (auth_is_authenticated()) : ?>
                    <ul class="nav navbar-nav navbar-right">
                        <li class="dropdown">
                            <a href="#" class="dropdown-toggle profile-image" data-toggle="dropdown">
                                <span><?php echo auth_get_name(); ?></span>
<?php if (!empty(auth_get_image())) : ?>
                                <img class="img-circle avatar-image-nav" src="<?php echo auth_get_image(); ?>">
<?php else: ?>
                                <i class="mdi-action-account-circle avatar-icon-nav"></i>
<?php endif; ?>
                            </a>
                            <ul class="dropdown-menu">
<?php if (!empty(auth_get_image())) : ?>
                                <li class="hidden-xs"><div><img class="img-responsive center-block avatar-image" src="<?php echo auth_get_image(); ?>"></div></li>
<?php else: ?>
                                <li class="hidden-xs"><div><i class="mdi-action-account-box avatar-icon"></i></div></li>
<?php endif; ?>
                                <li><a><strong><?php echo auth_get_name(); ?></strong></a></li>
                                <li><a><?php echo auth_get_email(); ?></a></li>
                                <li class="divider"></li>
                                <li><a href="auth/?action=logout">Sign-out</a></li>
                            </ul>
                        </li>
                    </ul>
<?php endif; ?>
                </div>
            </div> <!-- ./container -->
        </nav> <!-- ./navbar -->

        <div class="container">
<?php if ($display_type == DISPLAY_TYPE_RESULT) : ?>
            <div class="panel panel-success">
                <div class="panel-heading">
                    <h2 class="panel-title" style="font-size:20px;">Listing file</h2>
                </div>
                <div class="panel-body">
                    <pre><code><?php echo $lst_contents; ?></code></pre>
                </div>
            </div>
            <div class="panel panel-success">
                <div class="panel-heading">
                    <h2 class="panel-title" style="font-size:20px;">Object code</h2>
                </div>
                <div class="panel-body">
                    <pre><code><?php echo $obj_contents; ?></code></pre>
                </div>
            </div>

            <p><a href="javascript:history.back()" class="btn btn-primary btn-material-indigo-600">Go back</a></p>
<?php elseif ($display_type == DISPLAY_TYPE_ERROR) : ?>
            <div class="panel panel-danger">
                <div class="panel-heading">
                    <h2 class="panel-title" style="font-size:20px;">Error!</h2>
                </div>
                <div class="panel-body">
                    <p>The provided input code failed to assemble. The assembler reported ther error below.</p>
                    <pre><code><?php echo $error_msg; ?></code></pre>
                </div>
            </div>

            <div class="panel panel-info">
                <div class="panel-heading">
                    <h2 class="panel-title" style="font-size:20px;">Original code</h2>
                </div>
                <div class="panel-body">
                    <pre><code><?php echo $asm_contents; ?></code></pre>
                </div>
            </div>

            <p><a href="javascript:history.back()" class="btn btn-primary btn-material-indigo-600">Go back</a></p>
<?php elseif ($display_type == DISPLAY_TYPE_INPUT) : ?>
            <div class="panel panel-success">
                <div class="panel-heading">
                    <h2 class="panel-title" style="font-size:20px;">Input</h2>
                </div>
                <div class="panel-body">
                    <form action="" method="post">
                        <div class="form-group">
                            <label for="asmcode" class="col-lg-2 control-label">Assembly code</label>
                            <div class="col-lg-10">
                                <textarea id="asmcode" class="form-control" type="text" name="assembly" rows="10" placeholder="Paste your SIC/XE assembly code here"></textarea>
                            </div>
                        </div>
                        <div class="form-group">
                            <div class="col-lg-10 col-lg-offset-2">
                                <button id="clearasm" type="button" class="btn btn-default">Clear</button>
                                <button type="submit" class="btn btn-primary btn-material-indigo-600">Assemble!</button>
                            </div>
                        </div>
                    </form>
                </div>
            </div>
<?php elseif ($display_type == DISPLAY_TYPE_LOGIN) : ?>
            <div class="panel panel-warning">
                <div class="panel-heading">
                    <h2 class="panel-title" style="font-size:20px;">Login</h2>
                </div>
                <div class="panel-body">
                    <p>Please log in to access the SIC/XE Assembler.</p>
                    <a class="btn btn-block btn-social btn-github" href="auth/?action=login&amp;provider=github">
                        <i class="fa fa-github"></i> Sign in with GitHub
                    </a>
                    <a class="btn btn-block btn-social btn-bitbucket disabled" href="auth/bitbucket">
                        <i class="fa fa-bitbucket"></i> Sign in with Bitbucket
                    </a>
                    <a class="btn btn-block btn-social btn-google" href="auth/?action=login&amp;provider=google">
                        <i class="fa fa-google"></i> Sign in with Google
                    </a>
                </div>
            </div>
<?php endif; ?>
        </div>

        <!-- jQuery 2.x JS -->
        <script type="text/javascript" src="bower_components/jquery/dist/jquery.min.js"></script>

        <!-- Bootstrap JS -->
        <script type="text/javascript" src="bower_components/bootstrap/dist/js/bootstrap.min.js"></script>

        <!-- Material Design JS -->
        <script type="text/javascript" src="bower_components/bootstrap-material-design/dist/js/material.min.js"></script>
        <script type="text/javascript" src="bower_components/bootstrap-material-design/dist/js/ripples.min.js"></script>

        <!-- Autosize JS -->
        <script type="text/javascript" src="bower_components/autosize/dest/autosize.min.js"></script>

        <script>
            $.material.init();
        </script>

        <!-- Line numbering -->
        <script>
            $('pre').each(function () {
                var $preElem = $(this);
                $preElem.prepend('<span class="line-number"></span>');
                var numLines = $preElem.html().split(/\n/).length;
                for (var j = 0; j < numLines; j++) {
                    $preElem.find('span').first().append('<span>' + (j + 1) + '</span>');
                }
            });
        </script>

        <script>
            // Apply autosize js to assembly code input text area
            autosize($("#asmcode"));

            $("button#clearasm").click(function() {
                var $asmcode = $("#asmcode");
                $asmcode.val('');

                // Trigger resize after clear
                var evt = document.createEvent('Event');
                evt.initEvent('autosize.update', true, false);
                asmcode.dispatchEvent(evt);
            });
        </script>
    </body>
</html>
