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

if( empty($_GET["name"]) ) die($current_module->language->add->name_not_provided);
if( trim($settings->get($_GET["name"])) != "" ) die($current_module->language->add->name_already_exists);

$settings->set($_GET["name"], "");
echo "OK";
