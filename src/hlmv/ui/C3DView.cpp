#include <memory>

#include <wx/image.h>

#include "hlmv/CHLMVSettings.h"

#include "common/CGlobals.h"

#include "graphics/GraphicsHelpers.h"

#include "soundsystem/CSoundSystem.h"

#include "ui/CwxOpenGL.h"

#include "C3DView.h"

wxBEGIN_EVENT_TABLE( C3DView, wxGLCanvas )
	EVT_PAINT( C3DView::Paint )
	EVT_MOUSE_EVENTS( C3DView::MouseEvents )
wxEND_EVENT_TABLE()

const float C3DView::FLOOR_SIDE_LENGTH = 200;

C3DView::C3DView( wxWindow* pParent, CHLMVSettings* const pSettings, I3DViewListener* pListener )
	: wxGLCanvas( pParent, wxOpenGL().GetCanvasAttributes(), wxID_ANY, wxDefaultPosition, wxSize( 600, 400 ) )
	, m_pSettings( pSettings )
	, m_pListener( pListener )
{
	m_pContext = wxOpenGL().GetContext( this );
}

C3DView::~C3DView()
{
	SetCurrent( *m_pContext );

	DestroyUVFrameBuffer();

	glDeleteTexture( m_GroundTexture );
	glDeleteTexture( m_BackgroundTexture );

	m_pContext = nullptr;
}

void C3DView::Paint( wxPaintEvent& event )
{
	SetCurrent( *m_pContext );

	//Can't use the DC to draw anything since OpenGL draws over it.
	wxPaintDC( this );

	glClearColor( 0.5, 0.5, 0.5, 1.0 );

	const wxSize size = GetClientSize();
	
	if( m_pSettings->mirror )
	{
		glClearStencil( 0 );

		glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT );
	}
	else
		glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

	glViewport( 0, 0, size.GetX(), size.GetY() );

	if( m_pSettings->showTexture )
		DrawTexture( m_pSettings->texture, m_pSettings->textureScale, m_pSettings->showUVMap, m_pSettings->overlayUVMap, m_pSettings->antiAliasUVLines, m_pSettings->pUVMesh );
	else
		DrawModel();

	if( m_pListener )
		m_pListener->Draw3D( size );

	glFlush();
	SwapBuffers();
}

void C3DView::MouseEvents( wxMouseEvent& event )
{
	//Ignore input in weapon origin mode.
	//TODO: refactor
	if( m_pSettings->useWeaponOrigin || m_pSettings->showTexture )
	{
		event.Skip();
		return;
	}

	if( event.ButtonDown() )
	{
		m_flOldRotX = m_pSettings->rot[ 0 ];
		m_flOldRotY = m_pSettings->rot[ 1 ];
		VectorCopy( m_pSettings->trans, m_vecOldTrans );
		m_flOldX = event.GetX();
		m_flOldY = event.GetY();
		m_pSettings->pause = false;

		m_iButtonsDown |= event.GetButton();
	}
	else if( event.ButtonUp() )
	{
		m_iButtonsDown &= ~event.GetButton();
	}
	else if( event.Dragging() )
	{
		if( event.LeftIsDown() && m_iButtonsDown & wxMOUSE_BTN_LEFT )
		{
			if( event.GetModifiers() & wxMOD_SHIFT )
			{
				m_pSettings->trans[ 0 ] = m_vecOldTrans[ 0 ] - ( float ) ( event.GetX() - m_flOldX );
				m_pSettings->trans[ 1 ] = m_vecOldTrans[ 1 ] + ( float ) ( event.GetY() - m_flOldY );
			}
			else
			{
				m_pSettings->rot[ 0 ] = m_flOldRotX + ( float ) ( event.GetY() - m_flOldY );
				m_pSettings->rot[ 1 ] = m_flOldRotY + ( float ) ( event.GetX() - m_flOldX );
			}
		}
		else if( event.RightIsDown() && m_iButtonsDown & wxMOUSE_BTN_RIGHT )
		{
			m_pSettings->trans[ 2 ] = m_vecOldTrans[ 2 ] + ( float ) ( event.GetY() - m_flOldY );
		}

		Refresh();
	}
	else
	{
		event.Skip();
	}
}

void C3DView::PrepareForLoad()
{
	SetCurrent( *m_pContext );
}

void C3DView::UpdateView()
{
	//TODO: move logic out of this class and into another
	const double flCurTime = GetCurrentTime();

	float flFrameTime = flCurTime - Globals.GetPreviousRealTime();

	Globals.SetRealTime( flCurTime );

	if( flFrameTime > 1.0f )
		flFrameTime = 0.1f;

	if( flFrameTime < 1.0 / Globals.GetFPS() )
		return;

	Globals.SetPreviousTime( Globals.GetCurrentTime() );
	Globals.SetCurrentTime( Globals.GetCurrentTime() + flFrameTime );
	Globals.SetFrameTime( flFrameTime );
	Globals.SetPreviousRealTime( Globals.GetRealTime() );

	if( m_pSettings->playSequence && m_pSettings->GetStudioModel() )
	{
		//TODO: set directly
		m_pSettings->GetStudioModel()->SetFrameRate( m_pSettings->speedScale );
		const float flDeltaTime = m_pSettings->GetStudioModel()->AdvanceFrame( /*flFrameTime * m_pSettings->speedScale*/ );

		//TODO: put this listener elsewhere
		class CStudioModelListener final : public IAnimEventHandler
		{
		public:
			CStudioModelListener( CHLMVSettings* const pSettings )
				: m_pSettings( pSettings )
			{
			}

			void HandleAnimEvent( const CAnimEvent& event ) override final
			{
				switch( event.iEvent )
				{
				case SCRIPT_EVENT_SOUND:			// Play a named wave file
				case SCRIPT_EVENT_SOUND_VOICE:
					{
						//TODO: define PITCH_NORM
						if( m_pSettings->playSound )
						{
							soundSystem().PlaySound( event.pszOptions, 1.0f, 100 );
						}

						break;
					}

				default: break;
				}
			}

			CHLMVSettings* const m_pSettings;
		};

		CStudioModelListener listener( m_pSettings );

		m_pSettings->GetStudioModel()->DispatchAnimEvents( listener, flDeltaTime );
	}

	if( !m_pSettings->pause )
		Refresh();
}

void C3DView::SetupRenderMode( RenderMode renderMode )
{
	if( renderMode == RenderMode::INVALID )
		renderMode = m_pSettings->renderMode;

	graphics::helpers::SetupRenderMode( renderMode );
}

void C3DView::DrawTexture( const int iTexture, const float flTextureScale, const bool bShowUVMap, const bool bOverlayUVMap, const bool bAntiAliasLines, const mstudiomesh_t* const pUVMesh )
{
	const wxSize size = GetClientSize();

	graphics::helpers::DrawTexture( size.GetX(), size.GetY(), *m_pSettings->GetStudioModel(), iTexture, flTextureScale, bShowUVMap, bOverlayUVMap, bAntiAliasLines, pUVMesh );
}

void C3DView::DrawModel()
{
	const wxSize size = GetClientSize();

	//
	// draw background
	//

	if( m_pSettings->showBackground && m_BackgroundTexture != GL_INVALID_TEXTURE_ID && !m_pSettings->showTexture )
	{
		graphics::helpers::DrawBackground( m_BackgroundTexture );
	}

	if( !m_pSettings->GetStudioModel() )
		return;

	graphics::helpers::SetProjection( size.GetX(), size.GetY() );

	glMatrixMode( GL_MODELVIEW );
	glPushMatrix();
	glLoadIdentity();

	if( m_pSettings->useWeaponOrigin )
	{
		glTranslatef( -m_pSettings->weaponOrigin[ 0 ], -m_pSettings->weaponOrigin[ 1 ], -m_pSettings->weaponOrigin[ 2 ] );

		glRotatef( -90, 1.0f, 0.0f, 0.0f );
		glRotatef( 90, 0.0f, 0.0f, 1.0f );
	}
	else
	{
		glTranslatef( -m_pSettings->trans[ 0 ], -m_pSettings->trans[ 1 ], -m_pSettings->trans[ 2 ] );

		glRotatef( m_pSettings->rot[ 0 ], 1.0f, 0.0f, 0.0f );
		glRotatef( m_pSettings->rot[ 1 ], 0.0f, 0.0f, 1.0f );
	}

	g_vright[ 0 ] = g_vright[ 1 ] = m_pSettings->trans[ 2 ];

	m_pSettings->drawnPolys = 0;

	// setup stencil buffer and draw mirror
	if( m_pSettings->mirror )
	{
		m_pSettings->drawnPolys += graphics::helpers::DrawMirroredModel( *m_pSettings->GetStudioModel(), m_pSettings->renderMode, 
																		 m_pSettings->renderSettings, m_pSettings->wireframeOverlay, FLOOR_SIDE_LENGTH );
	}

	SetupRenderMode();

	glCullFace( GL_FRONT );
	m_pSettings->drawnPolys += m_pSettings->GetStudioModel()->DrawModel( m_pSettings->renderSettings );

	//Draw wireframe overlay
	if( m_pSettings->wireframeOverlay )
	{
		m_pSettings->drawnPolys += graphics::helpers::DrawWireframeOverlay( *m_pSettings->GetStudioModel(), m_pSettings->renderSettings );
	}

	//
	// draw ground
	//

	if( m_pSettings->showGround )
	{
		graphics::helpers::DrawFloor( FLOOR_SIDE_LENGTH, m_GroundTexture, m_pSettings->groundColor, m_pSettings->mirror );
	}

	glPopMatrix();
}

void C3DView::CreateUVFrameBuffer()
{
	if( m_UVFrameBuffer == 0 )
	{
		glGenFramebuffers( 1, &m_UVFrameBuffer );

		glBindFramebuffer( GL_FRAMEBUFFER, m_UVFrameBuffer );

		glGenTextures( 1, &m_UVRenderTarget );
	}
}

void C3DView::DestroyUVFrameBuffer()
{
	SetCurrent( *m_pContext );

	if( m_UVFrameBuffer != 0 )
	{
		glDeleteFramebuffers( 1, &m_UVFrameBuffer );
		m_UVFrameBuffer = 0;
	}
}

void C3DView::SetUVRenderTargetDimensions( const int iWidth, const int iHeight )
{
	if( m_UVRenderTarget == GL_INVALID_TEXTURE_ID )
		return;

	glBindTexture( GL_TEXTURE_2D, m_UVRenderTarget );

	glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB, iWidth, iHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr );

	//Poor filtering, so it doesn't mess with the results.
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );

	//Unbind it so there can be no conflicts.
	glBindTexture( GL_TEXTURE_2D, GL_INVALID_TEXTURE_ID );

	glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_UVRenderTarget, 0 );

	const GLenum drawBuffer = GL_COLOR_ATTACHMENT0;

	glDrawBuffers( 1, &drawBuffer );
}

bool C3DView::LoadBackgroundTexture( const wxString& szFilename )
{
	UnloadBackgroundTexture();

	m_BackgroundTexture = wxOpenGL().glLoadImage( szFilename.c_str() );

	//TODO: notify UI
	m_pSettings->showBackground = m_BackgroundTexture != GL_INVALID_TEXTURE_ID;

	return m_BackgroundTexture != GL_INVALID_TEXTURE_ID;
}

void C3DView::UnloadBackgroundTexture()
{
	glDeleteTexture( m_BackgroundTexture );
}

bool C3DView::LoadGroundTexture( const wxString& szFilename )
{
	glDeleteTexture( m_GroundTexture );

	m_GroundTexture = wxOpenGL().glLoadImage( szFilename.c_str() );

	return m_GroundTexture != GL_INVALID_TEXTURE_ID;
}

void C3DView::UnloadGroundTexture()
{
	glDeleteTexture( m_GroundTexture );
}

/*
*	Saves the given texture's UV map.
*/
void C3DView::SaveUVMap( const wxString& szFilename, const int iTexture )
{
	const studiohdr_t* const pHdr = m_pSettings->GetStudioModel()->getTextureHeader();

	if( !pHdr )
		return;

	const mstudiotexture_t& texture = ( ( mstudiotexture_t* ) ( ( byte* ) pHdr + pHdr->textureindex ) )[ iTexture ];

	SetCurrent( *m_pContext );

	//Create the framebuffer if it doesn't exist yet.
	CreateUVFrameBuffer();

	SetUVRenderTargetDimensions( texture.width, texture.height );

	glBindFramebuffer( GL_FRAMEBUFFER, m_UVFrameBuffer );

	const GLenum completeness = glCheckFramebufferStatus( GL_FRAMEBUFFER );

	if( completeness != GL_FRAMEBUFFER_COMPLETE )
	{
		//TODO: show error code as string
		wxMessageBox( "UV map framebuffer is incomplete!" );
		return;
	}

	std::unique_ptr<byte[]> rgbData = std::make_unique<byte[]>( texture.width * texture.height * 3 );

	glViewport( 0, 0, texture.width, texture.height );

	glClearColor( 0.0f, 0.0f, 0.0f, 1.0f );

	glClear( GL_COLOR_BUFFER_BIT );

	graphics::helpers::DrawTexture( texture.width, texture.height, *m_pSettings->GetStudioModel(), iTexture, 1.0f, true, false, false, m_pSettings->pUVMesh );

	glFlush();
	glFinish();

	glReadPixels( 0, 0, texture.width, texture.height, GL_RGB, GL_UNSIGNED_BYTE, rgbData.get() );

	glBindFramebuffer( GL_FRAMEBUFFER, 0 );

	//We have to flip the image vertically, since OpenGL reads it upside down.
	std::unique_ptr<byte[]> correctedData = std::make_unique<byte[]>( texture.width * texture.height * 3 );

	for( int y = 0; y < texture.height; ++y )
	{
		for( int x = 0; x < texture.width; ++x )
		{
			for( int i = 0; i < 3; ++i )
			{
				correctedData[ ( x + y * texture.width ) * 3 + i ] = rgbData[ ( x + ( texture.height - y - 1 ) * texture.width ) * 3 + i ];
			}
		}
	}

	wxImage image( texture.width, texture.height, correctedData.get(), true );

	if( !image.SaveFile( szFilename, wxBITMAP_TYPE_BMP ) )
	{
		wxMessageBox( wxString::Format( "Failed to save image \"%s\"!", szFilename.c_str() ) );
	}
}