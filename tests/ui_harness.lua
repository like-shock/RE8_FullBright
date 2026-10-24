callbacks = {}
draws = 0
re = { on_draw_ui = function(f) table.insert(callbacks, f) end }
imgui = {
    tree_node = function() draws = draws + 1; return true end,
    tree_pop = function() end,
    text = function(_) end,
    checkbox = function(label, value)
        if label == 'Fullbright' then return true, true end
        return false, value
    end,
    combo = function(_, index, items)
        assert(items[index] == 'None', 'REFramework combo uses Lua table keys')
        return select_last_key or false, select_last_key and #items or index
    end
}
