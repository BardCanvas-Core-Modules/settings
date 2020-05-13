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
include_once __DIR__ . "/local_functions.inc";

$current_module->load_extensions("specs_renderer", "prestart");

$var_groups = array();

# Engine settings are for strict admins
if( $account->level >= config::ADMIN_USER_LEVEL )
{
    foreach($language->settings_data->specs as $spec)
    {
        if( $spec["is_separator"] )
            $var_groups["engine"][trim($spec)] = (object) array(
                "name"             => "engine." . trim($spec),
                "value"            => "___SEPARATOR___",
                "display_group"    => trim($spec["display_group"]),
                "required_modules" => trim($spec["required_modules"]),
            );
        else
            $var_groups["engine"][trim($spec["key"])] = (object) array(
                "name"             => "engine." . trim($spec["key"]),
                "value"            => "",
                "display_group"    => trim($spec["display_group"]),
                "required_modules" => trim($spec["required_modules"]),
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
        {
            $var_groups[$template_group_key][trim($spec)] = (object) array(
                "name"             => $template_group_key . "." . trim($spec),
                "value"            => "___SEPARATOR___",
                "display_group"    => trim($spec["display_group"]),
                "required_modules" => trim($spec["required_modules"]),
            );
            
            continue;
        }
        
        $include_setting      = true;
        $raw_required_modules = trim($spec["required_modules"]);
        if( ! empty($raw_required_modules) )
        {
            $required_modules = explode(",", $raw_required_modules);
            foreach($required_modules as $required_module_name)
            {
                if( ! $modules[$required_module_name]->enabled )
                {
                    $include_setting = false;
                    break;
                }
            }
        }
        
        if( $include_setting ) $var_groups[$template_group_key][trim($spec["key"])] = (object) array(
            "name"             => $template_group_key . "." . trim($spec["key"]),
            "value"            => "",
            "display_group"    => trim($spec["display_group"]),
            "required_modules" => $raw_required_modules,
        );
    }
}

$skipped_groups = array();
$display_groups = array();
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
    
    /** @var \SimpleXMLElement $spec */
    foreach($module->language->settings_data->specs as $spec)
    {
        $display_group = trim($spec["display_group"]);
        $actual_group  = $module->name . ":" . $display_group;
        if( ! empty($display_group) )
            if( ! in_array($actual_group, $display_groups) )
                $display_groups[] = $actual_group;
        
        if( ! empty($spec["required_modules"]) )
        {
            $required_modules = explode(",", trim($spec["required_modules"]));
            foreach($required_modules as $required_module_name)
                if( ! $modules[$required_module_name]->enabled )
                    $spec->addAttribute("disabled", "true");
        }
        
        if( trim($spec["disabled"]) == "true" ) continue;
        
        $key = trim($spec["key"]); if( empty($key) ) $key = trim($spec);
        $var_groups["modules:{$module->name}"][$key] = (object) array(
            "name"             => "modules:{$module->name}.{$key}",
            "value"            => $spec["is_separator"] ? "___SEPARATOR___" : "",
            "display_group"    => trim($spec["display_group"]),
            "required_modules" => trim($spec["required_modules"]),
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

<!-- Pseudo dialog -->
<link rel="stylesheet" href="<?= $config->full_root_path ?>/media/pseudo_dialog.css">

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
            
            var editor = CodeMirror.fromTextArea(element, {
                // viewPortMargin: Infinity,
                lineNumbers:    true,
                mode:           'htmlmixed'
            });
            
            $(this).data('editor', editor);
        });
        
        $('textarea.CodeMirror.wrapped').each(function()
        {
            var element = $(this).get(0);
            
            var editor = CodeMirror.fromTextArea(element, {
                // viewPortMargin: Infinity,
                lineNumbers:    true,
                lineWrapping:   true,
                mode:           'htmlmixed'
            });
            
            $(this).data('editor', editor);
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
    
    function open_settings_display_group(module, key, title)
    {
        if( typeof title === 'undefined' ) title = '';
        
        var sel = sprintf('#dg-%s-%s', module, key);
        $(sel).find('.title .caption').text(title);
        
        $('body').css('overflow', 'hidden');
        $(sel).fadeIn('fast', function()
        {
            $(sel).find('textarea.CodeMirror').each(function() { $(this).data('editor').refresh(); });
        });
    }
    
    function close_settings_display_group(module, key)
    {
        var sel = sprintf('#dg-%s-%s', module, key);
        $(sel).fadeOut('fast');
        $('body').css('overflow', 'auto');
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
        
        <? render_settings_tabs(); ?>
        
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
        
        if( empty($group_language->settings_data) )
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
                <?
                foreach($group_data as $key => $data) render_settings_group($key, $data);
                ?>
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
    
    <?
    if( ! empty($display_groups) ):
        foreach($display_groups as $display_group):
            list($module_name, $group_key) = explode(":", $display_group);
            ?>
            <div class="pseudo_dialog" id="<?= "dg-{$module_name}-{$group_key}" ?>">
                <div class="frame" style="max-width: calc(100vw - 10px)">
                    <div class="title ui-widget-header">
                        <i class="fa fa-times fa-lg fa-border pull-right"
                           style="cursor: pointer; margin-top: 5px; margin-right: 5px;"
                           onclick="close_settings_display_group('<?= $module_name ?>', '<?= $group_key ?>')"></i>
                        <span class="caption"></span>
                    </div>
                    <div class="content" style="max-height: calc(100vh - 65px)">
                        
                        <?
                        foreach($var_groups as $group_name => $group_data):
                            
                            /** @var \SimpleXMLElement $group_language */
                            if( $group_name == "engine"              ) $group_language = $language;
                            elseif($group_name == $template_group_key) $group_language = $template->language;
                            else                                       $group_language = $modules[str_replace("modules:", "", $group_name)]->language;
                            
                            if( empty($group_language->settings_data->title) && isset($modules[str_replace("modules:", "", $group_name)]) )
                                continue;
                            
                            if( empty($group_language->settings_data) )
                                continue;
                            
                            $group_language = $group_language->settings_data;
                            $details_class = "";
                            $group_style   = ""; # ($selected_group != "" && $group_name != $selected_group) ? "display: none;" : "";
                            ?>
                            
                            <table class="settings_group nav_table no_highlight">
                                <tbody>
                                <?
                                foreach($group_data as $key => $data)
                                {
                                    render_settings_group($key, $data, "modules:$module_name", $group_key, false, false);
                                }
                                ?>
                                </tbody>
                            </table>
                            
                        <? endforeach; ?>
                        
                    </div>
                </div>
            </div>
            <?
        endforeach;
    endif;
    ?>
    
</form>
