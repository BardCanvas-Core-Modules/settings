<?php
/**
 * @var array             $errors
 * @var array             $messages
 * @var config            $config
 * @var \SimpleXMLElement $language
 */

use hng2_base\config;

$var_groups = array();

foreach($language->settings_data->specs as $spec)
{
    if( $spec["is_separator"] )
        $var_groups["engine"][trim($spec)] = (object) array(
            "name"  => "engine." . trim($spec),
            "value" => "___SEPARATOR___"
        );
    else
        $var_groups["engine"][trim($spec["key"])] = (object) array(
            "name"  => "engine." . trim($spec["key"]),
            "value" => ""
        );
}

foreach($modules as $module)
{
    if( empty($module->language->settings_data->specs) ) continue;
    
    foreach($module->language->settings_data->specs as $spec)
        $var_groups["modules:{$module->name}"][trim($spec["key"])] = (object) array(
            "name"  => "modules:{$module->name}." . trim($spec["key"]),
            "value" => ""
        );
}

$query = "
    select * from settings
    where name not like 'modules:%.installed'
    and   name not like 'modules:%.enabled'
    order by name asc
";
$res = $database->query($query);
while($row = $database->fetch_object($res))
{
    preg_match('/^(.*)\.(.*)$/', $row->name, $matches);
    
    $group_name = $matches[1];
    $var_key    = $matches[2];
    
    if( isset($var_groups[$group_name][$var_key]) )
        $var_groups[$group_name][$var_key]->value = $row->value;
    else
        $var_groups[$group_name][$var_key] = (object) array(
            "name"  => $row->name,
            "value" => $row->value,
        );
}

$undefined_vars = array();
?>

<!-- Extender inits -->
<? $current_module->load_extensions("specs_renderer", "preinits"); ?>

<!-- Expandible Text Area -->
<script type="text/javascript" src="<?=$config->full_root_path?>/lib/jquery.exptextarea.js"></script>
<script type="text/javascript"> $(document).ready(function() { $('.expandible_textarea').expandingTextArea(); }); </script>

<!-- CodeMirror -->
<script                 src="<?=$config->full_root_path?>/lib/CodeMirror-5.16.2/lib/codemirror.js"></script>
<link rel="stylesheet" href="<?=$config->full_root_path?>/lib/CodeMirror-5.16.2/lib/codemirror.css">
<style type="text/css">
    .CodeMirror {
        border: 1px solid silver;
        height: auto;
        font-family: 'Lucida Console', 'Courier new', Courier, monospace !important; font-size: 8pt;
        /*max-width: 99%;*/
    }
    td.indented { padding-left: 18px; }
</style>

<script type="text/javascript">
    $(document).ready(function()
    {
        $('textarea.CodeMirror').each(function()
        {
            var element = $(this).get(0);
            
            CodeMirror.fromTextArea(element, {
                viewPortMargin: Infinity,
                lineNumbers:    true
            }); 
        });
        
        function prepare_form_submission()
        {
            $.blockUI(blockUI_default_params);
            stop_notifications_getter();
        }
        
        function process_form_response(text)
        {
            if( text != 'OK' )
            {
                alert(text);
                
                $.unblockUI();
                return;
            }
            
            location.href = '<?= $_SERVER["PHP_SELF"] ?>?wasuuup=<?= md5(mt_rand(1, 65535)) ?>';
        }
        
        $('#settings_form').ajaxForm({
            target:       '#settings_saving_target',
            beforeSubmit: prepare_form_submission,
            success:      process_form_response
        });
    });
</script>

<h1 class="clearfix">
    <button class="pull-right" onclick="location.href = '<?= $_SERVER["PHP_SELF"] ?>?wasuuup=<?= md5(mt_rand(1, 65535)) ?>'">
        <span class="fa fa-refresh"></span>
        <?= $language->reload ?>
    </button>
    <?= $current_module->language->admin->record_nav->page_title ?>
</h1>

<div id="settings_saving_target" style="display: none;"></div>

<form method="post" id="settings_form" action="<?= $_SERVER["PHP_SELF"] ?>?wasuuup=<?=md5(mt_rand(1,65535))?>">
    
    <? foreach($var_groups as $group_name => $group_data):
        
        /** @var \SimpleXMLElement $group_language */
        if( $group_name == "engine" ) $group_language = $language;
        else                          $group_language = $modules[str_replace("modules:", "", $group_name)]->language;
        
        if( empty($group_language->settings_data) )
        {
            $undefined_vars[$group_name] = $group_data;
            
            continue;
        }
        
        $group_language = $group_language->settings_data;
        $details_class = "";
        ?>
        
        <div class="table_wrapper">
            <table class="settings_group nav_table no_highlight">
                
                <thead>
                <tr>
                    <th colspan="2"><?= $group_language->title ?></th>
                </tr>
                </thead>
                
                <tbody>
                <? foreach($group_data as $key => $data):
                    
                    if( $data->value == "___SEPARATOR___" )
                    {
                        $details_class = "indented";
                        echo "
                        <tr class='clicked'>
                            <td colspan=\"2\">
                                <span class='fa fa-caret-down'></span>
                                <b>{$key}</b>
                            </td>
                        </tr>
                        ";
                        
                        continue;
                    }
                    
                    /** @var \SimpleXMLElement $specs */
                    $specs = current($group_language->xpath("//specs[@key='$key']"));
                    if( empty($specs) )
                    {
                        $undefined_vars[$group_name][$key] = $data;
                        
                        continue;
                    }
                    
                    if($specs["disabled"] == "true") continue;
                    
                    $class = "";
                    if($specs["is_locked"] == "true") $class .= " disabled";
                    if($specs["highlight_class"])     $class .= " {$specs["highlight_class"]}";
                    ?>
                    <tr class="<?= $class ?>">
                        <td width="70%" class="<?= $details_class ?>">
                            <div class="principal_data">
                                <? if($specs["is_locked"] == "true") echo "<span class='fa fa-lock'></span>" ?>
                                <?= $specs->title ?>
                                <span class="framed_content inlined fixed_font" style="font-weight: normal;"><?= $data->name ?></span>
                            </div>
                            <blockquote>
                                <?= $specs->description ?>
                            </blockquote>
                            <!--<span class="framed_content inlined"><?= $data->name ?></span>-->
                        </td>
                        <td class="control" width="30%">
                            <?
                            if($specs["is_locked"] != "true")
                                render_settings_editor($specs->type, $data->name, $data->value, $specs);
                            else
                                echo "<div class='framed_content disabled'>" 
                                    . ($data->value == "" ? "&mdash;" : $data->value) 
                                    . "</div>";
                            ?>
                        </td>
                    </tr>
                <? endforeach; ?>
                </tbody>
            </table>
            
            <p align="right" style="margin-top: 5px;">
                <button type="submit">
                    <span class="fa fa-save"></span>
                    <?= $current_module->language->admin->record_nav->save_now ?>
                </button>
            </p>
        </div>
    
    <? endforeach; ?>
    
    <? if( ! empty($undefined_vars)): ?>
        
        <div class="framed_content state_ko">
            <span class="fa fa-warning"></span>
            <?= $current_module->language->admin->undefined_settings ?>
        </div>
        
        <? foreach($undefined_vars as $group_name => $group_data): ?>
            
            <div class="table_wrapper">
                <table class="settings_group nav_table no_highlight">
                    
                    <thead>
                    <tr>
                        <th colspan="2"><?= $group_name ?></th>
                    </tr>
                    </thead>
                    
                    <tbody>
                    <? foreach($group_data as $key => $data): ?>
                        <tr>
                            <td width="70%">
                                <div class="principal_data"><?= ucwords(str_replace("_", " ", $key)) ?></div>
                                <span class="framed_content inlined fixed_font" style="font-weight: normal;"><?= $data->name ?></span>
                            </td>
                            <td width="30%">
                                <? render_settings_editor("textarea", $data->name, $data->value, null) ?>
                            </td>
                        </tr>
                    <? endforeach; ?>
                    </tbody>
                </table>
                
                <p align="right" style="margin-top: 5px;">
                    <button type="submit">
                        <span class="fa fa-save"></span>
                        <?= $current_module->language->admin->record_nav->save_now ?>
                    </button>
                </p>
            </div>
            
        <? endforeach; ?>
        
    <? endif; ?>
    
</form>

<?
$generation_index = 0;
function render_settings_editor($type, $name, $value, $specs = null)
{
    global $language, $generation_index, $current_module, $config;
    
    $generation_index++;
    $type = trim($type);
    
    if( $type == "list" )
    {
        echo "<select name='names[{$name}]'>";
        foreach($specs->options->option as $option)
        {
            $selected = trim($option["value"]) == $value ? "selected" : "";
            echo "<option {$selected} value='{$option["value"]}'>{$option}</option>";
        }
        echo "</select>";
        
        return;
    }
    
    if( $type == "boolean" )
    {
        $value     = $value == "true" ? "true" : "false";
        $style_on  = $value == "true" ? "" : "display: none;";
        $style_off = $value != "true" ? "" : "display: none;";
        echo "
            {$language->off}
            <span class='fa-pseudo-switch' data-value-on='true' data-value-off='false' onclick='toggle_fa_pseudo_switch(this);'>
                <input type='hidden' name='names[$name]' value='$value'>
                <span class='toggler toggle-on  fa fa-toggle-on'  style='$style_on'></span>
                <span class='toggler toggle-off fa fa-toggle-off' style='$style_off'></span>
            </span>
            {$language->on}
        ";
        
        return;
    }
    
    if( in_array($type, array("text", "email")) )
    {
        $value = htmlspecialchars($value);
        echo "<input " . " type='{$type}' name='names[{$name}]' value='{$value}'>";
        
        return;
    }
    
    if( $type == "template_selector" )
    {
        $installed_templates = glob(ABSPATH . "/templates/*", GLOB_ONLYDIR);
        
        echo "<select name='names[{$name}]'>";
        foreach($installed_templates as $template_dir)
        {
            $template_name = basename($template_dir);
            
            $selected = $template_name == $value ? "selected" : "";
            echo "<option {$selected} value='{$template_name}'>{$template_name}</option>";
        }
        echo "</select>";
        
        return;
    }
    
    if( $type == "yes/no" )
    {
        $value   = $value == "true" ? "true" : "false";
        $sel_on  = $value == "true" ? "checked" : "";
        $sel_off = $value != "true" ? "checked" : "";
        echo "
            <label><input type='radio' $sel_on  name='names[{$name}]' value='true'>{$language->yes}</label>
            <label><input type='radio' $sel_off name='names[{$name}]' value='false'>{$language->no}</label>
        ";
        
        return;
    }
    
    if( $type == "number" )
    {
        echo "
            <input type='number' name='names[{$name}]' value='{$value}' style='text-align: center;'
                   min='{$specs->options->min}' max='{$specs->options->max}'>
        ";
        
        return;
    }
    
    if( $type == "code" )
    {
        echo "
            <textarea class='CodeMirror' id='ta_$generation_index' name='names[{$name}]'>" . htmlspecialchars($value) . "</textarea>
        ";
        
        return;
    }
    
    if( substr($type, 0, 1) == "@" )
    {
        # Extender!
        preg_match('#^@(.*):(.*)/(.*)$#i', $type, $matches);
        if( empty($matches) )
        {
            echo "
                <div class='framed_content state_ko'>
                    <span class='fa fa-warning'></span>
                    {$current_module->language->admin->wrong_type}
                </div>
                <textarea class='expandible_textarea' name='names[{$name}]'>" . htmlspecialchars($value) . "</textarea>
            ";
            
            return;
        }
        else
        {
            $module_name = $matches[1];
            $hook_area   = $matches[2];
            $hook_marker = $matches[3];
            
            if( ! isset($current_module->extended_by[$module_name][$hook_area]->{$hook_marker}) )
            {
                $message = replace_escaped_vars(
                    $current_module->language->admin->missing_extender,
                    array('{$module}', '{$area}', '{$marker}'),
                    array($module_name, $hook_area, $hook_marker)
                );
                
                echo "
                    <div class='framed_content state_ko'>
                        <span class='fa fa-warning'></span>
                        {$message}
                    </div>
                    <textarea class='expandible_textarea' name='names[{$name}]'>" . htmlspecialchars($value) . "</textarea>
                ";
                
                return;
            }
            
            $config->globals["$hook_area/$hook_marker"] = array("name" => $name, "value" => $value);
            $current_module->load_extensions($hook_area, $hook_marker);
            
            return;
        }
    }
    
    # Default: textarea
    echo "
        <textarea class='expandible_textarea' name='names[{$name}]'>" . htmlspecialchars($value) . "</textarea>
    ";
}
?>
