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

try
{
    $config->bump_disk_cache();
    $config->bump_mem_cache();
}
catch(\Exception $e)
{
    die( $e->getMessage() );
}

send_notification($account->id_account, "success", sprintf(
    $current_module->language->admin->bump_all_caches->done,
    $config->disk_cache_version,
    $config->memory_cache_version
));

echo "OK";
