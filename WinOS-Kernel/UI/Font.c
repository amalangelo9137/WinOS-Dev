float Get_Font_Width(nk_handle handle, float height, const char* text, int len) {
    // We'll tell Nuklear each char is 14px wide so they aren't too far apart
    return (float)len * 14.0f;
}

void InitNuklear(BOOT_CONFIG* config) {
    struct nk_user_font* font = &nuklear_font;
    font->userdata = nk_handle_ptr(config->FontSpriteData);
    font->height = (float)config->FontGlyphSize;
    font->width = Get_Font_Width;

    nk_init_fixed_inherit_attributes(&ctx, font);
}