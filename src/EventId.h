/** \file       EventId.cpp
 *  \brief      Contains definition of the event id enums.
 *  \author     Khralkatorrix
 */

/**
 * Copyright (C) 2017 Khralkatorrix <https://github.com/kytulendu>
 *
 * This file is part of Gw2Browser.
 *
 * Gw2Browser is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

namespace gw2b {

    namespace {

        enum {
            ID_ShowFindFile = wxID_HIGHEST + 1, // Show find file window
            ID_ShowFileList,                    // Show file list window
            ID_ShowLog,                         // Show log window
            ID_ClearLog,                        // Clear the log window
            ID_CompareIndex,                    // Compare current index with another index file
            //ID_ResetLayout,
            //ID_SetBackgroundColor,
            //ID_ShowGrid,                      // Show grid on PreviewGLCanvas
            //ID_ShowMask,                      // Apply white texture with black background, no shader
            //ID_SetCanvasSize,                 // Set PreviewGLCanvas size
            ID_BtnFindFile,                     // Browser's find file button
            ID_BtnBack,                         // Sound player's back button
            ID_BtnPlay,                         // Sound player's play button
            ID_BtnStop,                         // Sound player's stop button
            ID_BtnForward,                      // Sound player's forward button
            ID_SliderVolume,                    // Sound player's volume slider
            ID_SliderPlayback,                  // Sound player's playback slider
        };

    };

}; // namespace gw2b
