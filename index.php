<?php
/**
 * Settings module admin index
 *
 * @package    HNG2
 * @subpackage settings
 * @author     Alejandro Caballero - lava.caballero@gmail.com
 */

include "../config.php";
include "../includes/bootstrap.inc";
if( ! $account->_is_admin ) throw_fake_404();

if( ! empty($_POST["names"]) )
{
    $messages = array();
    
    foreach($_POST["names"] as $key => $val)
    {
        $val = trim(stripslashes($val));
        list($group, $var) = explode(".", $key);
        if( $val != $settings->get($key) )
        {
            $settings->set($key, $val);
            $messages[] = replace_escaped_vars($current_module->language->admin->record_nav->errors->var_ok, '{$name}', ucwords(str_replace("_", " ", $var)));
        }
    }
    
    if( count($messages) > 0 )
    {
        send_notification($account->id_account, "success", implode("\n", $messages));
        
        die("OK");
    }
}

$template->page_contents_include = "index.nav.inc";
$template->set_page_title($current_module->language->admin->record_nav->page_title);
include "{$template->abspath}/admin.php";
