local standalone = path.absolute(os.scriptdir()) == path.absolute(os.projectdir())

if standalone then
    set_project("imgui-md2")
    set_version("0.1.0")
    set_languages("cxx17")
    add_rules("mode.debug", "mode.release")
    add_requires("imgui v1.92.8", {
        configs = {
            glfw = true,
            opengl3 = true,
            shared = false
        }
    })
end

option("imgui_md2_embed_full_fonts", {
    description = "Embed Roboto Light/Medium in addition to Regular and Material Icons",
    showmenu = true,
    default = false,
    type = "boolean"
})

target("imgui-md2")
    set_kind("static")
    add_files("src/*.cpp")
    add_headerfiles("include/(imgui_md2/*.h)")
    add_includedirs("include", {public = true})
    add_packages("imgui", {public = true})
    add_defines('IMGUI_MD2_ASSET_DIR="assets"')
    if has_config("imgui_md2_embed_full_fonts") then
        add_defines("IMGUI_MD2_EMBED_FULL_FONTS")
    end
    if is_plat("windows") then
        add_defines("NOMINMAX")
    end

if standalone then
    -- Dev-only tool: renders every component once and crops each capture to
    -- the widget's own item rect for docs/screenshots/. Not part of the
    -- published library; built on request only (xmake build imgui-md2-screenshots).
    target("imgui-md2-screenshots")
        set_kind("binary")
        set_default(false)
        add_deps("imgui-md2")
        add_files("tools/screenshot/main.cpp")
        if is_plat("windows") then
            add_defines("NOMINMAX")
            add_syslinks("opengl32", "gdi32", "user32", "shell32")
        end
end
