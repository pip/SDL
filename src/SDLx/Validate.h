
SV *obj_make( int size_ptr,  void* obj, char* CLASS )
{
    SV *   objref   = newSV( size_ptr );
    void** pointers = malloc(2 * sizeof(void*));
    pointers[0]     = (void*)obj;
    pointers[1]     = (void*)PERL_GET_CONTEXT;

    sv_setref_pv( objref, CLASS, (void *)pointers);

    return objref;
}

SV *rect( SV *rect)
{
    SV *retval = NULL;
    //we hand this over to perl to handle

    if( !SvOK(rect) )
    {
        SDL_Rect* r = safemalloc( sizeof(SDL_Rect) );
        r->x        = 0;
        r->y        = 0;
        r->w        = 0;
        r->h        = 0;
        retval      = obj_make( sizeof( SDL_Rect *), (void *)(r), "SDL::Rect" );
    }
    else if( sv_derived_from(rect, "ARRAY") )
    {
        SDL_Rect* r = safemalloc( sizeof(SDL_Rect) );
        int ra[4];
        int i       = 0;
        AV* recta   = (AV*)SvRV(rect);
        for(i = 0; i < 4; i++)
        { 
            SV* iv = av_shift(recta);
            if( !SvOK( iv ) || iv == &PL_sv_undef )
                ra[i] = 0;
            else
                ra[i] = SvIV( iv );
        }

        r->x   = ra[0]; r->y = ra[1]; r->w = ra[2]; r->h= ra[3];
        retval = obj_make( sizeof( SDL_Rect *), (void *)(r), "SDL::Rect" );
    }
    else if( sv_isobject(rect) && sv_derived_from(rect, "SDL::Rect") )
    {
        SvREFCNT_inc(rect); //Incase an anonymous rect is passed in. We should detect this some how.
        return rect;
    }
    else
        croak("Rect must be number or arrayref or SDL::Rect or undef");

    return retval;
}

SV *surface( SV *surface )
{
    if( sv_isobject(surface) && sv_derived_from(surface, "SDL::Surface"))
    {
        SvREFCNT_inc(surface);
        return surface;
    }
    croak("Surface must be SDL::Surface or SDLx::Surface");
    return NULL;
}

SDL_Color *bag_to_color( SV *bag )
{
    SDL_Color *color = NULL;

    if( sv_isobject(bag) && (SvTYPE(SvRV(bag)) == SVt_PVMG) )
    {
        void **pointers = (void **)(SvIV((SV *)SvRV( bag ))); 
        color           = (SDL_Color *)(pointers[0]);
    }
    
    return color;
}

char *_color_format( SV *color )
{
    char *retval = NULL;
    if( !SvOK(color) || SvIOK(color) )
        retval = "number";
    else if( sv_derived_from(color, "ARRAY") )
        retval = "arrayref";
    else if( sv_isobject(color) && sv_derived_from(color, "SDL::Color") )
        retval = "SDLx::Color";
    else
        croak("Color must be number or arrayref or SDLx::Color");

    return retval;
}

SV *_color_number( SV *color, SV *alpha )
{
    int    c      = SvIV(color);
    int    a      = SvIV(alpha);
    Uint32 retval = SvUV(color);

    if( !SvOK(color) || color < 0 )
    {
        if( color < 0 )
        croak("Color was a negative number");
        retval = a == 1 ? 0x000000FF : 0;
    }
    else
    {
        if( a == 1 && (c > 0xFFFFFFFF) )
        {
            croak("Color was number greater than maximum expected: 0xFFFFFFFF");
            retval = 0xFFFFFFFF; 
        }
        else if ( a != 1 && ( c > 0xFFFFFF) )
        {
            croak("Color was number greater than maximum expected: 0xFFFFFF");
            retval = 0xFFFFFF;
        }
    }

    return newSVuv(retval);
}

AV *_color_arrayref( AV *color, SV *alpha )
{
    AV *retval = newAV();
    int length = SvTRUE(alpha) ? 4 : 3;
    int i      = 0;
    for(i = 0; i < length; i++)
    {
        SV *c = *av_fetch(color, i, 0);

        if( av_len(color) < i || !SvOK(*av_fetch(color, i, 0)) )
            av_push(retval, newSVuv(i == 3 ? 0xFF : 0));
        else
        {
            int c = SvIV(*av_fetch(color, i, 0));
            if( c > 0xFF )
            {
                croak("Number in color arrayref was greater than maximum expected: 0xFF");
                av_push(retval, newSVuv(0xFF));
            }
            else if( c < 0 )
            {
                croak("Number in color arrayref was negative");
                av_push(retval, newSVuv(0));
            }
            else
                av_push(retval, newSVuv(c));
        }
    }
    sv_2mortal((SV*)retval);
    return retval;
}