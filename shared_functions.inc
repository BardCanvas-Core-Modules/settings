<?

/**
 * @var int $generation_index Used to keep track of codemirror renders
 */
$generation_index = 0;

function render_settings_editor($type, $name, $value, $specs = null, $input_name = "names")
{
    global $language, $generation_index, $modules, $config, $database;
    
    $current_module = $modules["settings"];
    
    $generation_index++;
    $type = trim($type);
    
    if( $type == "list" )
    {
        echo "<select name='{$input_name}[{$name}]'>";
        foreach($specs->options->option as $option)
        {
            $selected = trim($option["value"]) == $value ? "selected" : "";
            echo "<option {$selected} value='{$option["value"]}'>{$option}</option>";
        }
        echo "</select>";
        
        return;
    }
    
    if( $type == "user_level_selector" )
    {
        $default = trim($specs->options->default);
        if( empty($default) ) $default = 10;
        if( empty($value) ) $value = $default;
        
        echo "<select name='{$input_name}[{$name}]'>";
        foreach($config->user_levels_by_level as $level => $title)
        {
            if( $level <= 1 ) continue;
            
            $selected = $level == $value ? "selected" : "";
            echo "<option {$selected} value='{$level}'>{$level} - {$title}</option>";
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
                <input type='hidden' name='{$input_name}[$name]' value='$value'>
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
        echo "<input " . " type='{$type}' name='{$input_name}[{$name}]' value='{$value}'>";
        
        return;
    }
    
    if( $type == "template_selector" )
    {
        $installed_templates = glob(ROOTPATH . "/templates/*", GLOB_ONLYDIR);
        
        echo "<select name='{$input_name}[{$name}]'>";
        foreach($installed_templates as $template_dir)
        {
            $xml_file = "{$template_dir}/template_info.xml";
            if( ! file_exists($xml_file) ) continue;
            
            $template_key  = basename($template_dir);
            $template_xml  = simplexml_load_file($xml_file);
            $template_name = "{$template_xml->name} {$template_xml->version}";
            
            $selected = $template_key == $value ? "selected" : "";
            echo "<option {$selected} value='{$template_key}'>{$template_name}</option>";
        }
        echo "</select>";
        
        return;
    }
    
    if( $type == "locale_list" )
    {
        $options           = array();
        $installed_locales = glob(ROOTPATH . "/language/*.xml");
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
        
        echo "<select name='{$input_name}[{$name}]'>";
        foreach($options as $iso => $caption)
        {
            $selected = $iso == $value ? "selected" : "";
            echo "<option {$selected} value='{$iso}'>{$caption}</option>";
        }
        echo "</select>";
        
        return;
    }
    
    if( $type == "country" )
    {
        $options = array();
        $res     = $database->query("select * from countries order by name");
        while( $row = $database->fetch_object($res) ) $options[$row->alpha_2] = $row->name;
        
        $selected = empty($value) ? "selected" : "";
        echo "<select name='{$input_name}[{$name}]'>";
        echo "<option {$selected} value=''>{$current_module->language->please_select_an_option}</option>";
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
            <label><input type='radio' $sel_on  name='{$input_name}[{$name}]' value='true'>{$language->yes}</label>
            <label><input type='radio' $sel_off name='{$input_name}[{$name}]' value='false'>{$language->no}</label>
        ";
        
        return;
    }
    
    if( $type == "number" )
    {
        echo "
            <input type='number' name='{$input_name}[{$name}]' value='{$value}' style='text-align: center;'
                   min='{$specs->options->min}' max='{$specs->options->max}'>
        ";
        
        return;
    }
    
    if( $type == "code" )
    {
        echo "
            <textarea class='CodeMirror' id='ta_$generation_index' name='{$input_name}[{$name}]'>" . htmlspecialchars($value) . "</textarea>
        ";
        
        return;
    }
    
    if( $type == "button" )
    {
        # Note: the "onclick" option must include a "return false" statement or the form will be submitted!
        echo "
            <button style='{$specs->options->style}' onclick='{$specs->options->onclick}'>{$specs->options->caption}</button>
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
                <textarea class='expandible_textarea' name='{$input_name}[{$name}]'>" . htmlspecialchars($value) . "</textarea>
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
                    <textarea class='expandible_textarea' name='{$input_name}[{$name}]'>" . htmlspecialchars($value) . "</textarea>
                ";
                
                return;
            }
            
            $config->globals["$hook_area/$hook_marker"] = array(
                "name" => $name, "value" => $value, "options" => $specs->options
            );
            $current_module->load_extensions($hook_area, $hook_marker);
            
            return;
        }
    }
    
    # Default: textarea
    echo "
        <textarea class='expandible_textarea' name='{$input_name}[{$name}]'>" . htmlspecialchars($value) . "</textarea>
    ";
}
?>
