/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012-2015 Jean-Pierre Charras, jean-pierre.charras@ujf-grenoble.fr
 * Copyright (C) 1992-2015 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef PCB_LAYER_BOX_SELECTOR_H
#define PCB_LAYER_BOX_SELECTOR_H

#include <widgets/layer_box_selector.h>

class PCB_BASE_FRAME;

/**
 * Class to display a pcb layer list in a wxBitmapComboBox.
 */
class PCB_LAYER_BOX_SELECTOR : public LAYER_BOX_SELECTOR
{
    PCB_BASE_FRAME* m_boardFrame;

    LSET m_layerMaskDisable;        // A mask to remove some (not allowed) layers
                                    // from layer list
    bool m_showNotEnabledBrdlayers; // true to list all allowed layers
                                    // (with not activated layers flagged)

public:
    // If you are thinking the constructor is a bit curious,
    // just remember it is used by automatically generated by wxFormBuilder files,
    // and it should mimic the wxBitmapComboBox constructor.
    // Therefore, value, style are not yet used,
    // but they are here for compatibility
    PCB_LAYER_BOX_SELECTOR( wxWindow* parent, wxWindowID id,
                        const wxString& value = wxEmptyString,
                        const wxPoint& pos = wxDefaultPosition,
                        const wxSize& size = wxDefaultSize,
                        int n = 0, const wxString choices[] = NULL, int style = 0 ) :
        LAYER_BOX_SELECTOR( parent, id, pos, size, n, choices )
    {
        m_boardFrame = NULL;
        m_showNotEnabledBrdlayers = false;
    }

    // Accessors

    // SetBoardFrame should be called after creating a PCB_LAYER_BOX_SELECTOR
    // It is not passed through the constructor because when using wxFormBuilder
    // we should use a constructor compatible with a wxBitmapComboBox
    void SetBoardFrame( PCB_BASE_FRAME* aFrame ) { m_boardFrame = aFrame; };

    // SetLayerSet allows disableing some layers, which are not
    // shown in list
    void SetNotAllowedLayerSet( LSET aMask ) { m_layerMaskDisable = aMask; }

    // Reload the Layers names and bitmaps
    // Virtual function
    void Resync() override;

    // Allow (or not) the layers not activated for the current board to be shown
    // in layer selector. Not actavated layers are flagged
    // ( "(not activated)" added to the layer name )
    void ShowNonActivatedLayers( bool aShow )
    {
        m_showNotEnabledBrdlayers = aShow;
    }

private:
    // Returns a color index from the layer id
    // Virtual function
    COLOR4D GetLayerColor( LAYER_NUM aLayer ) const override;

    // Returns true if the layer id is enabled (i.e. is it should be displayed)
    // Virtual function
    bool IsLayerEnabled( LAYER_NUM aLayer ) const override;

    // Returns the name of the layer id
    // Virtual function
    wxString GetLayerName( LAYER_NUM aLayer ) const override;

    LSET getEnabledLayers() const;
};

#endif // PCB_LAYER_BOX_SELECTOR_H
