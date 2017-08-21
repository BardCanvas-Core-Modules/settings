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

include_once __DIR__ . "/shared_functions.inc";

$var_groups = array();

# Engine settings are for strict admins
if( $account->level >= config::ADMIN_USER_LEVEL )
{
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
}

# Template settings - if the template has them!
$template_group_key = "templates:" . $template->name;
if( ! empty($template->language) )
{
    foreach($template->language->settings_data->specs as $spec)
    {
        if( $spec["is_separator"] )
            $var_groups[$template_group_key][trim($spec)] = (object) array(
                "name"  => $template_group_key . "." . trim($spec),
                "value" => "___SEPARATOR___"
            );
        else
            $var_groups[$template_group_key][trim($spec["key"])] = (object) array(
                "name"  => $template_group_key . "." . trim($spec["key"]),
                "value" => ""
            );
    }
}

$skipped_groups = array();
if( $account->level < config::ADMIN_USER_LEVEL ) $skipped_groups[] = "engine";
foreach($modules as $module)
{
    if( empty($module->language->settings_data) ) continue;
    if( empty($module->language->settings_data->specs) ) continue;
    
    $min_level = trim($module->language->settings_data["allow_admin_level"]);
    # echo "<p>[$min_level] --- [$account->level]</p>";
    if( empty($min_level) && $account->level < config::ADMIN_USER_LEVEL )
    {
        $skipped_groups[] = "modules:{$module->name}";
        
        continue;
    }
    if( ! empty($min_level) && $account->level < $min_level )
    {
        $skipped_groups[] = "modules:{$module->name}";
        
        continue;
    }
    
    foreach($module->language->settings_data->specs as $spec)
    {
        $key = trim($spec["key"]); if( empty($key) ) $key = trim($spec);
        $var_groups["modules:{$module->name}"][$key] = (object) array(
            "name"  => "modules:{$module->name}.{$key}",
            "value" => $spec["is_separator"] ? "___SEPARATOR___" : ""
        );
    }
}
# echo "<pre>" . print_r($var_groups, true) . "</pre>";

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
    
    if( in_array($group_name, $skipped_groups) ) continue;
    
    if( substr($group_name, 0, 10) == "templates:" && $group_name != $template_group_key ) continue;
    
    if( isset($var_groups[$group_name][$var_key]) )
        $var_groups[$group_name][$var_key]->value = $row->value;
    else
        if( $account->level >= config::ADMIN_USER_LEVEL )
            $var_groups[$group_name][$var_key] = (object) array(
                "name"  => $row->name,
                "value" => $row->value,
            );
}

$undefined_vars = array();
$selected_group = stripslashes($_GET["selected_group"]);
?>

<!-- Extender inits -->
<? $current_module->load_extensions("specs_renderer", "preinits"); ?>

<!-- Expandible Text Area -->
<script type="text/javascript" src="<?=$config->full_root_path?>/lib/jquery.exptextarea.js"></script>
<script type="text/javascript"> $(document).ready(function() { $('.expandible_textarea').expandingTextArea(); }); </script>

<!-- CodeMirror -->
<script                 src="<?=$config->full_root_path?>/lib/CodeMirror-5.16.2/lib/codemirror.js"></script>
<script                 src="<?=$config->full_root_path?>/lib/CodeMirror-5.16.2/mode/xml/xml.js"></script>
<script                 src="<?=$config->full_root_path?>/lib/CodeMirror-5.16.2/mode/javascript/javascript.js"></script>
<script                 src="<?=$config->full_root_path?>/lib/CodeMirror-5.16.2/mode/css/css.js"></script>
<script                 src="<?=$config->full_root_path?>/lib/CodeMirror-5.16.2/mode/htmlmixed/htmlmixed.js"></script>
<link rel="stylesheet" href="<?=$config->full_root_path?>/lib/CodeMirror-5.16.2/lib/codemirror.css">
<style type="text/css">
    .CodeMirror {
        border:      1px solid silver;
        height:      auto;
        font-family: 'Lucida Console', 'Courier new', Courier, monospace !important; font-size: 8pt;
        width:       40vw;
    }
    @media all and (max-width: 700px) { .CodeMirror { width: 100%; } }
    
    .CodeMirror-scroll { max-height: 80vh; }
    td.indented { padding-left: 18px; }
    
    .toplinks          { margin-bottom: 10px; }
    .toplinks .toplink { margin: 0 5px 5px 0; font-family: "Arial Narrow", Arial, Arial, Helvetica, sans-serif; }
    
    #settings_filter                    { display: inline-block; font-size: 24px; line-height: 24px; white-space: nowrap; }
    #settings_filter input[type="text"] { padding-right: 24px; }
    #settings_filter .cleanup           { position: absolute; margin-left: -24px; margin-top: 8px; cursor: pointer; color: ButtonText; }
    
    .highlight { background-color: yellow }
</style>

<!-- Hilitor -->
<script type="text/javascript" src="<?= $config->full_root_path ?>/lib/jquery.highlight-5.js"></script>

<script type="text/javascript">
    
    var selected_group = '<?= $selected_group == "*" ? $selected_group : wp_sanitize_filename($selected_group) ?>';
    
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
    
        location.href
            = $_PHP_SELF
            + '?selected_group=' + encodeURI(selected_group)
            + '&wasuuup='        + wasuuup();
    }
    
    $(document).ready(function()
    {
        $('textarea.CodeMirror:not(.wrapped)').each(function()
        {
            var element = $(this).get(0);
            
            CodeMirror.fromTextArea(element, {
                // viewPortMargin: Infinity,
                lineNumbers:    true,
                mode:           'htmlmixed'
            }); 
        });
        
        $('textarea.CodeMirror.wrapped').each(function()
        {
            var element = $(this).get(0);
            
            CodeMirror.fromTextArea(element, {
                // viewPortMargin: Infinity,
                lineNumbers:    true,
                lineWrapping:   true,
                mode:           'htmlmixed'
            });
        });
        
        var $form = $('#settings_form');
        $form.ajaxForm({
            target:       '#settings_saving_target',
            beforeSubmit: prepare_form_submission,
            success:      process_form_response
        });
        
        if( selected_group != '' )
            show_settings_group( $form.find('.toplink[data-group-name="' + selected_group + '"]') );
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
        selected_group = $trigger.attr('data-group-name');
        
        if( selected_group == '*' )
        {
            $toplinks.find('.toplink').toggleClass('state_active', false);
            $form.find('.group_container').show();
            
            return;
        }
        
        $toplinks.find('.toplink').toggleClass('state_active', false);
        $trigger.toggleClass('state_active', true);
        $form.find('.group_container:not([data-group-name="' +  selected_group + '"])').hide();
        $form.find('.group_container[data-group-name="' +  selected_group + '"]').show();
    }
    
    function add_setting()
    {
        var add_prompt = $('#add_prompt').text();
        var var_name   = prompt(add_prompt, '');
        if( var_name == null ) return;
        
        var url    = $_FULL_ROOT_PATH + '/settings/add_custom.php';
        var params = {
            name:    var_name,
            wasuuup: wasuuup()
        };
        $.blockUI(blockUI_default_params);
        $.get(url, params, function(response)
        {
            if( response != 'OK' )
            {
                alert(response);
                
                $.unblockUI();
                return;
            }
            
            location.href = $_FULL_ROOT_PATH + sprintf('/settings/?wasuuup=%s#orphan', wasuuup());
        });
    }
</script>

<div id="add_prompt" style="display: none;"><?= unindent($current_module->language->add->prompt) ?></div>

<a name="top"></a>
<h1 class="clearfix">
    <button class="pull-right" onclick="location.href = '<?= $_SERVER["PHP_SELF"] ?>?selected_group=' + selected_group + '&wasuuup=<?= md5(mt_rand(1, 65535)) ?>'">
        <span class="fa fa-refresh"></span>
        <?= $language->reload ?>
    </button>
    
    <?= $current_module->language->admin->record_nav->page_title ?>
    
    <button onclick="add_setting()">
        <span class="fa fa-plus"></span>
        <?= $current_module->language->add->caption ?>
    </button>
    
    <span id="settings_filter">
        <input type="text" placeholder="<?= $current_module->language->lookup_caption ?>"
               onkeyup="filter_settings()" onchange="filter_settings()">
        <img class="cleanup" onclick="clear_settings_filter()" src="<?= $config->full_root_path ?>/media/backspace.png">
    </span>
</h1>

<div id="settings_saving_target" style="display: none;"></div>

<form method="post" id="settings_form" action="<?= $_SERVER["PHP_SELF"] ?>?wasuuup=<?=md5(mt_rand(1,65535))?>">
    
    <input type="hidden" name="mode"       value="save">
    <input type="hidden" name="show_group" value="<?= $_POST["show_group"] ?>">
    
    <div class="toplinks" style="<? if(count($var_groups) <= 1) echo "display: none;"; ?>">
        <span class="toplink framed_content pseudo_link" data-group-name="*"
              onmouseover="$(this).toggleClass('state_hover', true)" onmouseout="$(this).toggleClass('state_hover', false)"
              onclick="show_settings_group(this)"><?= $current_module->language->admin->record_nav->show_all ?></span>
        
        <?
        $tabs = array();
        foreach($var_groups as $group_name => $group_data)
        {
            if( $group_name == "engine" ) continue;
            if( substr($group_name, 0, 10) == "templates:" ) continue;
            
                /** @var \SimpleXMLElement $group_language */
            $group_language = $modules[str_replace("modules:", "", $group_name )]->language;
            
            if( empty($group_language->settings_data) ) continue;
            if( empty($group_language->settings_data->title) ) continue;
            
            $group_language = $group_language->settings_data;
            $group_title    = trim($group_language->title);
            
            $tabs[$group_title] = $group_name;
        }
        
        ksort($tabs);
        ?>
        <? if( $account->level >= config::ADMIN_USER_LEVEL ): ?>
            <span class="toplink selectable framed_content pseudo_link" data-group-name="engine"
                  onmouseover="$(this).toggleClass('state_hover', true)" onmouseout="$(this).toggleClass('state_hover', false)"
                  onclick="show_settings_group(this)"><?= $language->settings_data->title ?></span>
        <? endif; ?>
        
        <? if( ! empty($template->language) ): ?>
            <span class="toplink selectable framed_content pseudo_link" data-group-name="<?= wp_sanitize_filename($template_group_key) ?>"
                  onmouseover="$(this).toggleClass('state_hover', true)" onmouseout="$(this).toggleClass('state_hover', false)"
                  onclick="show_settings_group(this)"><?= $template->language->settings_data->title ?></span>
        <? endif; ?>
        
        <? foreach($tabs as $group_title => $group_name):
            $class = ""; # $group_name == $selected_group ? "state_active" : ""; ?>
            <span class="toplink selectable framed_content <?= $class ?> pseudo_link" data-group-name="<?= wp_sanitize_filename($group_name) ?>"
                  onmouseover="$(this).toggleClass('state_hover', true)" onmouseout="$(this).toggleClass('state_hover', false)"
                  onclick="show_settings_group(this)"><?= $group_title ?></span>
        <? endforeach; ?>
    </div>
    
    <?
    foreach($var_groups as $group_name => $group_data):
        
        # echo "<pre>" . print_r($group_data, true) . "</pre>";
        
        /** @var \SimpleXMLElement $group_language */
        if( $group_name == "engine"              ) $group_language = $language;
        elseif($group_name == $template_group_key) $group_language = $template->language;
        else                                       $group_language = $modules[str_replace("modules:", "", $group_name)]->language;
        
        if( empty($group_language->settings_data->title) && isset($modules[str_replace("modules:", "", $group_name)]) )
            continue;
        
        if( $group_data->value != "___SEPARATOR___" && empty($group_language->settings_data) )
        {
            $undefined_vars[$group_name] = $group_data;
            
            continue;
        }
        
        $group_language = $group_language->settings_data;
        $details_class = "";
        $group_style   = ""; # ($selected_group != "" && $group_name != $selected_group) ? "display: none;" : "";
        ?>
        
        <div class="table_wrapper group_container" data-group-name="<?= wp_sanitize_filename($group_name) ?>" 
             data-raw-group-name="<?= $group_name ?>" style="<?= $group_style ?>">
            
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
                        <tr class='separator clicked'>
                            <td colspan=\"2\">
                                <button type=\"submit\" class=\"pull-right\" onclick=\"selected_group = '" . addslashes($group_name) . "';\">
                                    <span class=\"fa fa-save\"></span>
                                    {$current_module->language->admin->record_nav->save_now}
                                </button>
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
                        <td class="details searchable_target <?= $details_class ?>">
                            <div class="principal_data">
                                <? if($specs["is_locked"] == "true") echo "<span class='fa fa-lock'></span>" ?>
                                <?= $specs->title ?>
                                <span class="framed_content inlined fixed_font" style="font-weight: normal;"><?= $data->name ?></span>
                            </div>
                            <? if( isset($specs->options->check_if_php_function_exists) ):
                                $exists  = function_exists(trim($specs->options->check_if_php_function_exists->function_name));
                                $class   = $exists ? "state_active" : "state_ko";
                                $caption = $exists ? $specs->options->check_if_php_function_exists->if_exists
                                         : $specs->options->check_if_php_function_exists->if_not_exists;
                                $icon    = $exists ? "fa-check" : "fa-times"; ?>
                                <blockquote>
                                    <div class="framed_content <?= $class ?>">
                                        <i class="fa <?= $icon ?>"></i>
                                        <?= $caption ?>
                                    </div>
                                </blockquote>
                            <? endif; ?>
                            <blockquote>
                                <?= replace_escaped_vars(
                                    $specs->description,
                                    array('{$full_root_url}'),
                                    array($config->full_root_url)
                                ) ?>
                            </blockquote>
                            <!--<span class="framed_content inlined"><?= $data->name ?></span>-->
                        </td>
                        <td class="control">
                            <?
                            if($specs["is_locked"] != "true")
                                render_settings_editor($specs->type, $data->name, $data->value, $specs);
                            else
                                echo "<div class='framed_content disabled'>" 
                                    . ($data->value == "" ? "&mdash;" : nl2br($data->value)) 
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
                <button type="submit" class="pull-right" onclick="selected_group = '<?= addslashes($group_name) ?>';">
                    <span class="fa fa-save"></span>
                    <?= $current_module->language->admin->record_nav->save_now ?>
                </button>
            </p>
        </div>
        
    <? endforeach; ?>
    
    <? if( ! empty($undefined_vars)): ?>
        
        <a name="orphan"></a>
        <div data-raw-group-name="__orphan__"></div>
        
        <? $warning_echoed = false; ?>
        <? foreach($undefined_vars as $group_name => $group_data):
            $group_style = ""; # ($selected_group != "" && $group_name != "__orphan__") ? "display: none;" : "";?>
            
            <div class="table_wrapper group_container" data-group-name="__orphan__" style="<?= $group_style ?>">
                
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
                            <td class="details searchable_target">
                                <span class="pull-right">
                                    <label>
                                        <input type="checkbox" name="deletes[]" value="<?= $data->name ?>" style="display: none;"
                                               onchange="var $this = $(this); var checked = $this.prop('checked'); $this.closest('label').toggleClass('critical', checked);">
                                        <i class="fa fa-trash fa-fw"></i>
                                    </label>
                                </span>
                                <div class="principal_data"><?= ucwords(str_replace("_", " ", $key)) ?></div>
                                <span class="framed_content inlined fixed_font" style="font-weight: normal;"><?= $data->name ?></span>
                            </td>
                            <td class="control">
                                <textarea class='CodeMirror' name='names[<?= $data->name ?>]'><?= htmlspecialchars($data->value) ?></textarea>
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
                    <button type="submit" class="pull-right" onclick="selected_group = '__orphan__';">
                        <span class="fa fa-save"></span>
                        <?= $current_module->language->admin->record_nav->save_now ?>
                    </button>
                </p>
            </div>
            
        <? endforeach; ?>
        
        <script type="text/javascript">
            var html = "<span class=\"toplink selectable framed_content pseudo_link\" data-group-name=\"__orphan__\" "
                + "onmouseover=\"$(this).toggleClass('state_hover', true)\" "
                + "onmouseout=\"$(this).toggleClass('state_hover', false)\" "
                + "onclick=\"show_settings_group(this)\">"
                + "<?= $current_module->language->admin->record_nav->orphan ?>"
                + "</span>";
            $('#settings_form').find('.toplinks').append(html);
        </script>
        
    <? endif; ?>
    
</form>
