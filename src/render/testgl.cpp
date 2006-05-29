#include <stdlib.h>
#include <stdio.h>
#include <string>

#include "rSDL.h"


#ifdef SDL_GL_DOUBLEBUFFER
#endif

#define HAVE_OPENGL 1


#ifdef HAVE_OPENGL
#ifdef WIN32
#include <windows.h>
#endif
#include <rGL.h>


int HandleEvent(SDL_Event *tEvent)
{
    int done;

    done = 0;
    switch( tEvent->type ) {
    case SDL_ACTIVEEVENT:
        /* See what happened */
        printf( "app %s ", tEvent->active.gain ? "gained" : "lost" );
        if ( tEvent->active.state & SDL_APPACTIVE ) {
            printf( "active " );
        } else if ( tEvent->active.state & SDL_APPMOUSEFOCUS ) {
            printf( "mouse " );
        } else if ( tEvent->active.state & SDL_APPINPUTFOCUS ) {
            printf( "input " );
        }
        printf( "focus\n" );
        break;

    case SDL_KEYDOWN:
        if( tEvent->key.keysym.sym == SDLK_ESCAPE ) {
            done = 1;
        }
        printf("key '%s' pressed\n",
               SDL_GetKeyName(tEvent->key.keysym.sym));
        break;
    case SDL_QUIT:
        done = 1;
        break;
    }
    return(done);
}

int RunGLTest( int argc, char* argv[] )
{
    int i;
    int w = 640;
    int h = 480;
    int bpp = 16;
    double color = 0.0;
    int box = 50;
    int x0 = w / 2 - box;
    int x1 = w / 2 + box;
    int y0 = h / 2 - box;
    int y1 = h / 2 + box;
    int done = 0;
    float cube[8][3]= {{ 0.5,  0.5, -0.5},
                       { 0.5, -0.5, -0.5},
                       {-0.5, -0.5, -0.5},
                       {-0.5,  0.5, -0.5},
                       {-0.5,  0.5,  0.5},
                       { 0.5,  0.5,  0.5},
                       { 0.5, -0.5,  0.5},
                       {-0.5, -0.5,  0.5}};
    Uint32 video_flags;
    int value;

    if( SDL_Init( SDL_INIT_VIDEO ) < 0 ) {
        fprintf(stderr,"Couldn't initialize SDL: %s\n",SDL_GetError());
        exit( 1 );
    }

    /* Set the flags we want to use for setting the video mode */
    video_flags = SDL_OPENGL;

    for ( i=1; argv[i]; ++i ) {
        if ( strcmp(argv[1], "-fullscreen") == 0 ) {
            video_flags |= SDL_FULLSCREEN;
        }
    }

    /* Initialize the display */
    SDL_GL_SetAttribute( SDL_GL_RED_SIZE, 5 );
    SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, 5 );
    SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, 5 );
    SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, 16 );
    SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );
    if ( SDL_SetVideoMode( w, h, bpp, video_flags ) == NULL ) {
        fprintf(stderr, "Couldn't set GL mode: %s\n", SDL_GetError());
        SDL_Quit();
        exit(1);
    }

    SDL_GL_GetAttribute( SDL_GL_RED_SIZE, &value );
    printf( "SDL_GL_RED_SIZE: requetsed 5, got %d\n", value );
    SDL_GL_GetAttribute( SDL_GL_GREEN_SIZE, &value );
    printf( "SDL_GL_GREEN_SIZE: requested 5, got %d\n", value);
    SDL_GL_GetAttribute( SDL_GL_BLUE_SIZE, &value );
    printf( "SDL_GL_BLUE_SIZE: requested 5, got %d\n", value );
    SDL_GL_GetAttribute( SDL_GL_DEPTH_SIZE, &value );
    printf( "SDL_GL_DEPTH_SIZE: requested 16, got %d\n", value );
    SDL_GL_GetAttribute( SDL_GL_DOUBLEBUFFER, &value );
    printf( "SDL_GL_DOUBLEBUFFER: requested 1, got %d\n", value );

    /* Set the window manager title bar */
    SDL_WM_SetCaption( "SDL GL test", "testgl" );

    glViewport( 0, 0, w, h );
    glMatrixMode( GL_PROJECTION );
    glLoadIdentity( );

    glOrtho( -2.0, 2.0, -2.0, 2.0, -20.0, 20.0 );

    glMatrixMode( GL_MODELVIEW );
    glLoadIdentity( );

    glEnable(GL_DEPTH_TEST);

    glDepthFunc(GL_LESS);

    /* Loop until done. */
    while( !done ) {
        GLenum glError;
        char* sdlError;
        SDL_Event tEvent;

        /* Do our drawing, too. */
        glClearColor( 0.0, 0.0, 0.0, 1.0 );
        glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glBegin( GL_QUADS );
        glColor3f(1.0, 0.0, 0.0);
        glVertex3fv(cube[0]);
        glVertex3fv(cube[1]);
        glVertex3fv(cube[2]);
        glVertex3fv(cube[3]);

        glColor3f(0.0, 1.0, 0.0);
        glVertex3fv(cube[3]);
        glVertex3fv(cube[4]);
        glVertex3fv(cube[7]);
        glVertex3fv(cube[2]);

        glColor3f(0.0, 0.0, 1.0);
        glVertex3fv(cube[0]);
        glVertex3fv(cube[5]);
        glVertex3fv(cube[6]);
        glVertex3fv(cube[1]);

        glColor3f(0.0, 1.0, 1.0);
        glVertex3fv(cube[5]);
        glVertex3fv(cube[4]);
        glVertex3fv(cube[7]);
        glVertex3fv(cube[6]);

        glColor3f(1.0, 1.0, 0.0);
        glVertex3fv(cube[5]);
        glVertex3fv(cube[0]);
        glVertex3fv(cube[3]);
        glVertex3fv(cube[4]);

        glColor3f(1.0, 0.0, 1.0);
        glVertex3fv(cube[6]);
        glVertex3fv(cube[1]);
        glVertex3fv(cube[2]);
        glVertex3fv(cube[7]);
        glEnd( );

        glMatrixMode(GL_MODELVIEW);
        glRotatef(5.0, 1.0, 1.0, 1.0);

        SDL_GL_SwapBuffers( );

        color += 0.01;

        if( color >= 1.0 ) {
            color = 0.0;
        }

        /* Check for error conditions. */
        glError = glGetError( );

        if( glError != GL_NO_ERROR ) {
            fprintf( stderr, "testgl: OpenGL error: %d\n", glError );
        }

        sdlError = SDL_GetError( );

        if( sdlError[0] != '\0' ) {
            fprintf(stderr, "testgl: SDL error '%s'\n", sdlError);
            SDL_ClearError();
        }

        /* Give the CPU some time to gather inputs. */
        SDL_Delay( 20 );

        /* Check if there's a pending tEvent. */
        while( SDL_PollEvent( &tEvent ) ) {
            done = HandleEvent(&tEvent);
        }
    }

    /* Destroy our GL context, etc. */
    SDL_Quit( );
}

int main(int argc, char *argv[])
{
    int i;
    int numtests;

    numtests = 1;
    for ( i=1; argv[i]; ++i ) {
        if ( strcmp(argv[1], "-twice") == 0 ) {
            ++numtests;
        }
    }
    for ( i=0; i<numtests; ++i ) {
        RunGLTest(argc, argv);
    }
    exit( 0 );
}

#else /* HAVE_OPENGL */

int main(int argc, char *argv[])
{
    printf("No OpenGL support on this system\n");
    exit(1);
}

#endif /* HAVE_OPENGL */
