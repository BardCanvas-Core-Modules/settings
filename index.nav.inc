<?php
/**
 * @var array             $errors
 * @var array             $messages
 * @var config            $config
 * @var \SimpleXMLElement $language
 * @var module            $current_module
 */

use hng2_base\config;
use hng2_base\module;

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

    .toplinks          { margin-bottom: 10px; }
    .toplinks .toplink { margin: 0 5px 5px 0; font-family: "Arial Narrow", Arial, Arial, Helvetica, sans-serif; }
    
    #settings_filter input[type="text"] { padding-right: 24px; }
    #settings_filter .cleanup           { position: absolute; margin-left: -24px; margin-top: 2px; cursor: pointer; color: ButtonText; }
    
    .highlight { background-color: yellow }
</style>

<!-- Hilitor -->
<script type="text/javascript" src="<?= $config->full_root_path ?>/lib/jquery.highlight-5.js"></script>

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
    
    function filter_settings()
    {
        var $form     = $('#settings_form');
        var $toplinks = $form.find('.toplinks');
        var $input    = $('#settings_filter').find('input[type="text"]');
        var value     = $input.val().trim().toLowerCase();
        
        $form.removeHighlight();
        if( value == '' )
        {
            $('.settings_group tr:hidden').show();
            $('.group_container:hidden').show();
            $toplinks.find('.toplink').toggleClass('state_active', false);
            $toplinks.show();
            
            return;
        }
        
        $toplinks.hide();
        $('.group_container').each(function()
        {
            var $group = $(this);
            var hidden_trs  = 0;
            var visible_trs = 0;
            $group.find('.settings_group tr.searchable').each(function()
            {
                var $tr      = $(this);
                var contents = $tr.text().toLowerCase();
                if( contents.indexOf(value) >= 0 )
                {
                    $tr.show();
                    visible_trs++;
                }
                else
                {
                    $tr.hide();
                    hidden_trs++;
                }
            });
            
            if( visible_trs == 0 ) $group.hide();
            else                   $group.show();
        });
        
        $('.searchable_target').highlight(value);
    }
    
    function clear_settings_filter()
    {
        $('#settings_filter').find('input[type="text"]').val('');
        filter_settings();
    }
    
    function show_settings_group(trigger)
    {
        var $form     = $('#settings_form');
        var $toplinks = $form.find('.toplinks');
        var $trigger  = $(trigger);
        var group_name = $trigger.attr('data-group-name');
        
        if( group_name == '*' )
        {
            $toplinks.find('.toplink').toggleClass('state_active', false);
            $form.find('.group_container').show();
            
            return;
        }
        
        $toplinks.find('.toplink').toggleClass('state_active', false);
        $trigger.toggleClass('state_active', true);
        $form.find('.group_container:not([data-group-name="' +  group_name + '"])').hide();
        $form.find('.group_container[data-group-name="' +  group_name + '"]').show();
    }
</script>

<a name="top"></a>
<h1 class="clearfix">
    <button class="pull-right" onclick="location.href = '<?= $_SERVER["PHP_SELF"] ?>?wasuuup=<?= md5(mt_rand(1, 65535)) ?>'">
        <span class="fa fa-refresh"></span>
        <?= $language->reload ?>
    </button>
    
    <?= $current_module->language->admin->record_nav->page_title ?>
    
    <span id="settings_filter">
        <input type="text" placeholder="<?= $current_module->language->lookup_caption ?>"
               onkeyup="filter_settings()" onchange="filter_settings()">
        <span class="cleanup fa fa-times fa-fw" onclick="clear_settings_filter()"></span>
    </span>
</h1>

<div id="settings_saving_target" style="display: none;"></div>

<form method="post" id="settings_form" action="<?= $_SERVER["PHP_SELF"] ?>?wasuuup=<?=md5(mt_rand(1,65535))?>">
    
    <div class="toplinks">
        <span class="toplink framed_content pseudo_link" data-group-name="*"
              onmouseover="$(this).toggleClass('state_hover', true)" onmouseout="$(this).toggleClass('state_hover', false)"
              onclick="show_settings_group(this)"><?= $current_module->language->admin->record_nav->show_all ?></span>
        
        <?
        foreach($var_groups as $group_name => $group_data):
            /** @var \SimpleXMLElement $group_language */
            if( $group_name == "engine" ) $group_language = $language;
            else                          $group_language = $modules[str_replace("modules:", "", $group_name)]->language;
            
            if( empty($group_language->settings_data) ) continue;
            
            $group_language = $group_language->settings_data;
            $group_title    = $group_language->title;
            ?>
            <span class="toplink framed_content pseudo_link" data-group-name="<?= sanitize_file_name($group_name) ?>"
                  onmouseover="$(this).toggleClass('state_hover', true)" onmouseout="$(this).toggleClass('state_hover', false)"
                  onclick="show_settings_group(this)"><?= $group_title ?></span>
        <? endforeach; ?>
    </div>
    
    <?
    foreach($var_groups as $group_name => $group_data):
        
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
        
        <div class="table_wrapper group_container" data-group-name="<?= sanitize_file_name($group_name) ?>">
            
            <table class="settings_group nav_table no_highlight">
                
                <thead>
                <tr>
                    <th colspan="2" class="searchable_target"><?= $group_language->title ?></th>
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
                    <tr class="searchable <?= $class ?>">
                        <td width="70%" class="searchable_target <?= $details_class ?>">
                            <div class="principal_data">
                                <? if($specs["is_locked"] == "true") echo "<span class='fa fa-lock'></span>" ?>
                                <?= $specs->title ?>
                                <span class="framed_content inlined fixed_font" style="font-weight: normal;"><?= $data->name ?></span>
                            </div>
                            <blockquote>
                                <?= replace_escaped_vars(
                                    $specs->description,
                                    array('{$full_root_url}'),
                                    array($config->full_root_url)
                                ) ?>
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
            
            <p class="clearfix" style="margin-top: 5px;">
                <a href="#top">
                   <span class="fa fa-arrow-up"></span>
                    <?= $current_module->language->back_to_top ?>
                </a>
                <button type="submit" class="pull-right">
                    <span class="fa fa-save"></span>
                    <?= $current_module->language->admin->record_nav->save_now ?>
                </button>
            </p>
        </div>
        
    <? endforeach; ?>
    
    <? if( ! empty($undefined_vars)): ?>
        
        <? $warning_echoed = false; ?>
        <? foreach($undefined_vars as $group_name => $group_data): ?>
            
            <div class="table_wrapper group_container" data-group-name="@!orphan">
                
                <? if( ! $warning_echoed ): ?>
                    <div class="framed_content state_ko" style="margin-top: 0;">
                        <span class="fa fa-warning"></span>
                        <?= $current_module->language->admin->undefined_settings ?>
                    </div>
                    <? $warning_echoed = true; ?>
                <? endif; ?>
                
                <table class="settings_group nav_table no_highlight">
                    
                    <thead>
                    <tr>
                        <th colspan="2" class="searchable_target">
                            <?= $group_name ?>
                        </th>
                    </tr>
                    </thead>
                    
                    <tbody>
                    <? foreach($group_data as $key => $data): ?>
                        <tr class="searchable">
                            <td width="70%" class="searchable_target">
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
                
                <p class="clearfix" style="margin-top: 5px;">
                    <a href="#top">
                        <span class="fa fa-arrow-up"></span>
                        <?= $current_module->language->back_to_top ?>
                    </a>
                    <button type="submit" class="pull-right">
                        <span class="fa fa-save"></span>
                        <?= $current_module->language->admin->record_nav->save_now ?>
                    </button>
                </p>
            </div>
            
        <? endforeach; ?>
        
        <script type="text/javascript">
            var html = "<span class=\"toplink framed_content pseudo_link\" data-group-name=\"@!orphan\" "
                + "onmouseover=\"$(this).toggleClass('state_hover', true)\" "
                + "onmouseout=\"$(this).toggleClass('state_hover', false)\" "
                + "onclick=\"show_settings_group(this)\">"
                + "<?= $current_module->language->admin->record_nav->orphan ?>"
                + "</span>";
            $('#settings_form').find('.toplinks').append(html);
        </script>
        
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
    
    if( $type == "locale_list" )
    {
        $options           = array();
        $installed_locales = glob(ABSPATH . "/language/*.xml");
        foreach($installed_locales as $locale_file)
        {
            $xml     = simplexml_load_file($locale_file);
            $iso     = trim($xml->info->locale);
            $caption = trim($xml->info->full_name);
            $options[$iso] = $caption;
        }
        asort($options);
        $english = $options["en_US"];
        unset($options["en_US"]);
        $options = array_merge(array("en_US" => $english), $options);
        
        echo "<select name='names[{$name}]'>";
        foreach($options as $iso => $caption)
        {
            $selected = $iso == $value ? "selected" : "";
            echo "<option {$selected} value='{$iso}'>{$caption}</option>";
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
    
    /**
     * Here, the type is defined in the language file as follows:
     * <type>@gallery:specs_renderer/image_selector</type>
     * So take a look at the gallery extender above to see how to deal with this kind of control.
     */
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
