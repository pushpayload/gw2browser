/** \file       Readers/AFNTReader.cpp
 *  \brief      Contains definition of the AFNT reader class.
 *  \author     BoyC, Khralkatorrix
 */

/**
 * Copyright (C) 2019 Khralkatorrix <https://github.com/kytulendu>
 * Copyright (C) 2019 BoyC <https://twitter.com/BoyCcns>
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

#pragma once

#ifndef READERS_AFNTREADER_H_INCLUDED
#define READERS_AFNTREADER_H_INCLUDED

#include "FileReader.h"

#include <vector>

namespace gw2b {

    struct Glyph {
        uint32 character;
        uint32 width;
        uint32 height;
        wxImage data;
    };

    struct Font {
        uint32 id;
        uint8 extents_x;
        uint8 extents_y;
        std::vector<Glyph> glyphs;
    };

    struct BGR {
        uint8   b;
        uint8   g;
        uint8   r;
    };

    //----------------------------------------------------------------------------
    //      AFNT_RLEDecoder
    //----------------------------------------------------------------------------

    class AFNT_RLEDecoder {
        byte* m_stream = nullptr;
        size_t m_yOffset;

    public:
        uint32 m_width;
        uint32 m_height;
        byte m_image[65536];

    public:
        byte* DecompressNextCharacter( byte* p_data );
    };

    //----------------------------------------------------------------------------
    //      AFNTReader
    //----------------------------------------------------------------------------

    class AFNTReader : public FileReader {
        struct FontDescriptor;

    public:
        /** Constructor.
        *  \param[in]  p_data       Data to be handled by this reader.
        *  \param[in]  p_datFile    Reference to an instance of DatFile.
        *  \param[in]  p_fileType   File type of the given data. */
        AFNTReader( const Array<byte>& p_data, DatFile& p_datFile, ANetFileType p_fileType );
        /** Destructor. Clears all data. */
        virtual ~AFNTReader( );

        /** Gets the type of data contained in this file. Not to be confused with
        *  file type.
        *  \return DataType    type of data. */
        virtual DataType dataType( ) const override {
            return DT_BitmapFont;
        }
        /** Gets the data contained in the data owned by this reader.
        *  \return std::vector<Font>     Fonts. */
        std::vector<Font> getFont( ) const;

    }; // class AFNTReader

}; // namespace gw2b

#endif // READERS_AFNTREADER_H_INCLUDED
