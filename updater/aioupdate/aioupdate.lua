--[[ AioUpdate — a tiny companion addon that updates the AioHud plugin from its latest GitHub release.

     A compiled C++ plugin can't hot-swap or unload itself (it would crash mid-call), so this Lua addon does the
     dance the plugin can't: check the latest release, download the payload zip, //unload the plugin, extract the
     new files over plugins\ (the DLL is unlocked once unloaded), then //load it again. The user's plugins\AioHud\
     data\ (config, profiles) is never in the zip, so nothing is lost.

     Install : copy this folder to <Windower>\addons\aioupdate\ , then  //lua load aioupdate  (or add it to init.txt).
     Use     : //aioupdate    (checks + updates if a newer release exists ; the HUD blinks off ~3s during the reload)
]]

_addon.name     = 'AioUpdate'
_addon.author   = 'Tetsouo'
_addon.version  = '1.0'
_addon.commands = { 'aioupdate', 'aioup' }

local REPO    = 'Tetsouo/AioHud'
local wp      = windower.windower_path
local plugins = wp .. 'plugins\\'
local data    = plugins .. 'AioHud\\data\\'
local zip     = data .. 'cache\\update.zip'
local ps1     = windower.addon_path .. 'update.ps1'

local function log(s) windower.add_to_chat(207, 'AioUpdate \\cs(120,200,255)>>\\cr ' .. s) end

-- the installed build version, written by the plugin to data\version.txt each launch
local function installed()
    local f = io.open(data .. 'version.txt', 'r')
    if not f then return '0' end
    local v = (f:read('*a') or ''):gsub('%s', '')
    f:close()
    return v ~= '' and v or '0'
end

-- run update.ps1 in a given mode, return its trimmed stdout (a single status line)
local function run(mode)
    local cmd = ('powershell -NoProfile -ExecutionPolicy Bypass -File "%s" -Mode %s -Repo %s -Current "%s" -PluginsDir "%s" -Zip "%s" 2>&1')
        :format(ps1, mode, REPO, installed(), plugins, zip)
    local h = io.popen(cmd)
    if not h then return 'ERROR could-not-run-powershell' end
    local out = h:read('*a') or ''
    h:close()
    return (out:gsub('^%s+', ''):gsub('%s+$', ''))
end

windower.register_event('addon command', function()
    log('checking for updates...')
    local r = run('prepare')
    if r:find('^UPTODATE') then
        log('already up to date (v' .. installed() .. ').')
    elseif r:find('^READY') then
        local tag = r:match('READY%s+(%S+)') or '?'
        log('downloading v' .. tag .. ' -> reloading AioHud...')
        windower.send_command('unload AioHud')
        -- wait for the unload so the DLL is unlocked, THEN extract + reload
        coroutine.schedule(function()
            local a = run('apply')
            windower.send_command('load AioHud')   -- reload either way (the new build on OK, the old one on failure)
            if a:find('^OK') then log('\\cs(120,255,120)updated to v' .. tag .. '\\cr.')
            else log('\\cs(255,120,120)update failed\\cr: ' .. a .. '  (dual-box? //unload on the other client first)') end
        end, 3)
    else
        log('\\cs(255,120,120)check failed\\cr: ' .. (r ~= '' and r or 'no response — network / GitHub down?'))
    end
end)

log('ready — type \\cs(255,230,150)//aioupdate\\cr to check for updates.')
