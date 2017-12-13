<?php
use hng2_base\config;

function render_settings_tabs()
{
    global $var_groups, $modules, $account, $template, $language, $template_group_key;
    
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
    <? endforeach;
}

function render_settings_group(
        $key, $data, $name_prefix = "", $display_group = "", $searchable_rows = true, $show_submit_buttons = true
) {
    /** @var \SimpleXMLElement $group_language */
    global $group_name, $group_language, $current_module, $config, $undefined_vars, $details_class;
    
    if( ! empty($name_prefix) )
        if( substr($data->name, 0, strlen($name_prefix)) != $name_prefix )
            return;
    
    if( $data->value == "___SEPARATOR___" )
    {
        if( trim($data->display_group) == $display_group )
        {
            $details_class = "indented";
            $button_style  = $show_submit_buttons ? "" : "display: none;";
            echo "
            <tr class='separator clicked'>
                <td colspan=\"2\">
                    <button type=\"submit\" class=\"pull-right\" style='{$button_style}'
                            onclick=\"selected_group = '" . addslashes($group_name) . "';\">
                        <span class=\"fa fa-save\"></span>
                        {$current_module->language->admin->record_nav->save_now}
                    </button>
                    <span class='fa fa-caret-down'></span>
                    <b>{$key}</b>
                </td>
            </tr>
            ";
        }
        
        return;
    }
    
    /** @var \SimpleXMLElement $specs */
    $specs = current($group_language->xpath("//specs[@key='$key']"));
    
    if( empty($specs) )
    {
        $undefined_vars[$group_name][$key] = $data;
        
        return;
    }
    
    if($specs["disabled"] == "true") return;
    if(trim($specs["display_group"]) != $display_group) return;
    
    $class = $searchable_rows ? "searchable " : "";
    if($specs["is_locked"] == "true") $class .= " disabled";
    if($specs["highlight_class"])     $class .= " {$specs["highlight_class"]}";
    
    $searchable_target_class = $searchable_rows ? "searchable_target" : "";
    ?>
    <tr class="<?= $class ?>">
        <td class="details <?= $searchable_target_class ?> <?= $details_class ?>">
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
    <?
}
