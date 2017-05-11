<?php
/**
 * Settings module admin index
 *
 * @package    HNG2
 * @subpackage settings
 * @author     Alejandro Caballero - lava.caballero@gmail.com
 */

use hng2_base\config;

include "../config.php";
include "../includes/bootstrap.inc";

if( $account->level < config::COADMIN_USER_LEVEL ) throw_fake_404();

if( $_POST["mode"] == "save" )
{
    $messages = array();
    
    $settings->prepare_batch();
    foreach($_POST["names"] as $key => $val)
    {
        $val = trim(stripslashes($val));
        list($group, $var) = explode(".", $key);
        if( $val != $settings->get($key) )
        {
            $settings->set($key, $val);
            $messages[] = replace_escaped_objects(
                $current_module->language->admin->record_nav->errors->var_ok,
                array('{$name}' => ucwords(str_replace("_", " ", $var)))
            );
        }
    }
    
    if( is_array($_POST["deletes"]) )
    {
        foreach($_POST["deletes"] as $var)
        {
            $settings->delete($var);
            $messages[] = replace_escaped_objects(
                $current_module->language->admin->record_nav->errors->var_deleted,
                array('{$name}' => ucwords(str_replace("_", " ", $var)))
            );
        }
    }
    
    if( count($messages) > 0 )
    {
        $settings->commit_batch();
        send_notification($account->id_account, "success", implode("<br>\n", $messages));
    }
    
    die("OK");
}

$template->page_contents_include = "index.nav.inc";
$template->set_page_title($current_module->language->admin->record_nav->page_title);
include "{$template->abspath}/admin.php";
