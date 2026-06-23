/** \file       CategoryTree.h
 *  \brief      Contains definition of the .dat structure tree control.
 *  \author     Rhoot
 */

/**
 * Copyright (C) 2014-2019 Khralkatorrix <https://github.com/kytulendu>
 * Copyright (C) 2013 Till034 <https://github.com/Till034>
 * Copyright (C) 2012 Rhoot <https://github.com/rhoot>
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

#include "stdafx.h"

#include <vector>

#include "Data.h"
#include "Exporter.h"

#include "CategoryTree.h"

namespace gw2b {

    //----------------------------------------------------------------------------
    //      CategoryTreeImageList
    //----------------------------------------------------------------------------

    CategoryTreeImageList::CategoryTreeImageList()
        : wxImageList(16, 16, true, 2)
    {
        this->Add(loadImage(getPath("interface/icons/open_folder.png")));
        this->Add(loadImage(getPath("interface/icons/unknown.png")));
        this->Add(loadImage(getPath("interface/icons/closed_folder.png")));
        this->Add(loadImage(getPath("interface/icons/exe.png")));
        this->Add(loadImage(getPath("interface/icons/dll.png")));
        this->Add(loadImage(getPath("interface/icons/image.png")));
        this->Add(loadImage(getPath("interface/icons/text.png")));
        this->Add(loadImage(getPath("interface/icons/sound.png")));
        this->Add(loadImage(getPath("interface/icons/font.png")));
        this->Add(loadImage(getPath("interface/icons/font_bitmap.png")));
        this->Add(loadImage(getPath("interface/icons/video.png")));
        this->Add(loadImage(getPath("interface/icons/model.png")));
    }

    //============================================================================/

    CategoryTreeImageList::~CategoryTreeImageList( ) {
    }

    //----------------------------------------------------------------------------
    //      CategoryTree
    //----------------------------------------------------------------------------

    wxIMPLEMENT_DYNAMIC_CLASS( CategoryTree, wxTreeCtrl );

    CategoryTree::CategoryTree() {
    }

    CategoryTree::CategoryTree( wxWindow* p_parent, const wxPoint& p_location, const wxSize& p_size)
        : wxTreeCtrl( p_parent, wxID_ANY, p_location, p_size, wxTR_HIDE_ROOT | wxTR_TWIST_BUTTONS | wxTR_DEFAULT_STYLE | wxTR_MULTIPLE ) {
        // Initialize tree
        auto images = new CategoryTreeImageList( );
        this->AssignImageList( images );
        this->AddRoot( wxT( "Root" ) );

        // Hookup events
        this->Bind( wxEVT_TREE_ITEM_EXPANDING, &CategoryTree::onItemExpanding, this );
        this->Bind( wxEVT_TREE_ITEM_COLLAPSING, &CategoryTree::onItemCollapsing, this );
        this->Bind( wxEVT_TREE_SEL_CHANGED, &CategoryTree::onSelChanged, this );
        this->Bind( wxEVT_TREE_ITEM_MENU, &CategoryTree::onContextMenu, this );
        this->Bind( wxEVT_MENU, &CategoryTree::onExtractConvertedFiles, this, wxID_SAVE );
        this->Bind( wxEVT_MENU, &CategoryTree::onExtractRawFiles, this, wxID_SAVEAS );
    }

    //============================================================================/

    CategoryTree::~CategoryTree( ) {
        this->cancelBackgroundFill( );

        if ( m_index ) {
            m_index->removeListener( this );
        }

        for ( auto const& it : m_listeners ) {
            it->onTreeDestruction( *this );
        }
    }

    //============================================================================/

    int CategoryTree::OnCompareItems( const wxTreeItemId& p_item1, const wxTreeItemId& p_item2 ) {
        int Result = 0;
        auto itemData1 = static_cast<const CategoryTreeItem*>( this->GetItemData( p_item1 ) );
        auto itemData2 = static_cast<const CategoryTreeItem*>( this->GetItemData( p_item2 ) );

        if ( itemData1->dataType() == CategoryTreeItem::DT_Entry && itemData2->dataType() == CategoryTreeItem::DT_Entry ) {
            auto itemEntry1 = static_cast<const DatIndexEntry*>( itemData1->data() );
            auto itemEntry2 = static_cast<const DatIndexEntry*>( itemData2->data() );

            if ( itemEntry1->baseId() > itemEntry2->baseId() ) {
                Result = 1;
            } else if ( itemEntry1->baseId() < itemEntry2->baseId() ) {
                Result = -1;
            }
        }
        return Result;
    }

    //============================================================================/

    void CategoryTree::addEntry( const DatIndexEntry& p_entry ) {
        auto category = this->ensureHasCategory( *p_entry.category( ) );
        if ( category.IsOk( ) ) {
            if ( this->IsExpanded( category ) ) {
                auto node = this->addEntry( category, p_entry );
                this->SetItemData( node, new CategoryTreeItem( CategoryTreeItem::DT_Entry, &p_entry ) );
            } else {
                auto itemData = static_cast<CategoryTreeItem*>( this->GetItemData( category ) );
                if ( !itemData || itemData->dataType( ) != CategoryTreeItem::DT_Category ) {
                    return;
                }
                itemData->setDirty( true );
            }
        }
    }

    //============================================================================/

    wxTreeItemId CategoryTree::ensureHasCategory( const DatIndexCategory& p_category, bool p_force ) {
        wxTreeItemId parent;
        auto parentIsRoot = false;

        // Determine parent node
        if ( p_category.parent( ) ) {
            parent = this->ensureHasCategory( *p_category.parent( ), p_force );
        } else {
            parent = this->GetRootItem( );
            parentIsRoot = true;
        }

        if ( !p_force ) {
            // If parent is invalid, it means the tree isn't expanded and we shouldn't add this
            if ( !parentIsRoot && ( !parent.IsOk( ) || !this->IsExpanded( parent ) ) ) {
                return wxTreeItemId( );
            }
        } else if ( !parent.IsOk( ) ) {
            return wxTreeItemId( );
        }

        wxTreeItemIdValue cookie;
        auto child = this->GetFirstChild( parent, cookie );

        // Scan for existing node
        while ( child.IsOk( ) ) {
            auto data = static_cast<const CategoryTreeItem*>( this->GetItemData( child ) );
            if ( !data || data->dataType( ) != CategoryTreeItem::DT_Category ) {
                break;
            }
            if ( data->data( ) == &p_category ) {
                return child;
            }
            child = this->GetNextChild( parent, cookie );
        }

        // Node does not exist, add it
        auto thisNode = this->addCategoryEntry( parent, p_category.name( ) );
        auto itemData = new CategoryTreeItem( CategoryTreeItem::DT_Category, &p_category );
        itemData->setDirty( true );
        this->SetItemData( thisNode, itemData );
        // All category nodes have children
        this->SetItemHasChildren( thisNode );
        return thisNode;
    }

    //============================================================================/

    wxTreeItemId CategoryTree::addEntry( const wxTreeItemId& p_parent, const DatIndexEntry& p_entry ) {
        if ( p_entry.name( ).IsNumber( ) ) {
            ulong number;
            p_entry.name( ).ToULong( &number );
            return this->addNumberEntry( p_parent, p_entry, number );
        }

        // NaN :)
        return this->addTextEntry( p_parent, p_entry );
    }

    //============================================================================/

    wxTreeItemId CategoryTree::addNumberEntry( const wxTreeItemId& p_parent, const DatIndexEntry& p_entry, uint p_name ) {
        wxTreeItemIdValue cookie;
        auto child = this->GetFirstChild( p_parent, cookie );
        wxTreeItemId previous;

        while ( child.IsOk( ) ) {
            wxString text = this->GetItemText( child );
            if ( !text.IsNumber( ) ) {
                break;
            }
            // Convert to uint
            ulong number;
            text.ToULong( &number );
            // Compare
            if ( number > p_name ) {
                break;
            }
            // Move to next
            previous = child;
            child = this->GetNextChild( p_parent, cookie );
        }

        // This item should be first if there *is* something in this list, but previous is nothing
        if ( child.IsOk( ) && !previous.IsOk( ) ) {
            return this->InsertItem( p_parent, 0, p_entry.name( ), this->getImageForEntry( p_entry ) );
        }
        // This item should be squashed in if both child and previous are ok
        if ( child.IsOk( ) && previous.IsOk( ) ) {
            return this->InsertItem( p_parent, previous, p_entry.name( ), this->getImageForEntry( p_entry ) );
        }
        // If the above fails, it means we went through the entire list without finding a proper spot
        return this->AppendItem( p_parent, p_entry.name( ), this->getImageForEntry( p_entry ) );
    }

    //============================================================================/

    wxTreeItemId CategoryTree::addTextEntry( const wxTreeItemId& p_parent, const DatIndexEntry& p_entry ) {
        wxTreeItemIdValue cookie;
        wxTreeItemId previous;
        auto child = this->GetFirstChild( p_parent, cookie );

        while ( child.IsOk( ) ) {
            auto text = this->GetItemText( child );
            // Compare
            if ( text > p_entry.name( ) ) {
                break;
            }
            // Move to next
            previous = child;
            child = this->GetNextChild( p_parent, cookie );
        }

        // This item should be first if there *is* something in this list, but previous is nothing
        if ( child.IsOk( ) && !previous.IsOk( ) ) {
            return this->InsertItem( p_parent, 0, p_entry.name( ), this->getImageForEntry( p_entry ) );
        }
        // This item should be squashed in if both child and previous are ok
        if ( child.IsOk( ) && previous.IsOk( ) ) {
            return this->InsertItem( p_parent, previous, p_entry.name( ), this->getImageForEntry( p_entry ) );
        }
        // If the above fails, it means we went through the entire list without finding a proper spot
        return this->AppendItem( p_parent, p_entry.name( ), this->getImageForEntry( p_entry ) );
    }

    //============================================================================/

    wxTreeItemId CategoryTree::addCategoryEntry( const wxTreeItemId& p_parent, const wxString& p_displayName ) {
        wxTreeItemIdValue cookie;
        wxTreeItemId previous;
        auto child = this->GetFirstChild( p_parent, cookie );

        while ( child.IsOk( ) ) {
            auto text = this->GetItemText( child );
            // Compare
            // If both start with number compare them as numbers
            if (text.length() > 0 && p_displayName.length() > 0 && std::isdigit(text.GetChar(0)) && std::isdigit(p_displayName.GetChar(0))) {
                if (text.length() > p_displayName.length()) {
                    break;
                }
                if (text.length() == p_displayName.length()) {
                    for (size_t i = 0; i < text.length(); i++) {
                        if (text.GetChar(i) > p_displayName.GetChar(i)) {
                            goto CompareCategoryNestedBreak;
                        }
                        else if (text.GetChar(i) < p_displayName.GetChar(i)) {
                            break;
                        }
                    }
                }
            }
            // Else as string
            else if ( text > p_displayName ) {
                break;
            }
            auto data = static_cast<const CategoryTreeItem*>( this->GetItemData( child ) );
            if ( !data || data->dataType( ) != CategoryTreeItem::DT_Category ) {
                break;
            }
            // Move to next
            previous = child;
            child = this->GetNextChild( p_parent, cookie );
        }
        CompareCategoryNestedBreak:

        // This item should be first if there *is* something in this list, but previous is nothing
        if ( child.IsOk( ) && !previous.IsOk( ) ) {
            return this->InsertItem( p_parent, 0, p_displayName, CategoryTreeImageList::IT_ClosedFolder );
        }
        // This item should be squashed in if both child and previous are ok
        if ( child.IsOk( ) && previous.IsOk( ) ) {
            return this->InsertItem( p_parent, previous, p_displayName, CategoryTreeImageList::IT_ClosedFolder );
        }
        // If the above fails, it means we went through the entire list without finding a proper spot
        return this->AppendItem( p_parent, p_displayName, CategoryTreeImageList::IT_ClosedFolder );
    }

    //============================================================================/

    void CategoryTree::clearEntries( ) {
        // Stop any background fill: its category pointers belong to data that is
        // about to be deleted.
        this->cancelBackgroundFill( );

        this->DeleteAllItems( );
        this->AddRoot( wxT( "Root" ) );

        for ( auto const& it : m_listeners ) {
            it->onTreeCleared( *this );
        }
    }

    //============================================================================/

    Array<const DatIndexEntry*> CategoryTree::getSelectedEntries( ) const {
        wxArrayTreeItemIds ids;
        this->GetSelections( ids );
        if ( ids.GetCount( ) == 0 ) {
            return Array<const DatIndexEntry*>( );
        }

        // Doing this in two steps since reallocating takes far longer than iterating

        // Start with counting the total amount of entries
        uint count = 0;
        for ( uint i = 0; i < ids.Count( ); i++ ) {
            auto itemData = static_cast<const CategoryTreeItem*>( this->GetItemData( ids[i] ) );
            if ( itemData->dataType( ) == CategoryTreeItem::DT_Entry ) {
                count++;
            } else if ( itemData->dataType( ) == CategoryTreeItem::DT_Category ) {
                count += static_cast<const DatIndexCategory*>( itemData->data( ) )->numEntries( true );
            }
        }

        // Create and populate the array to return
        Array<const DatIndexEntry*> retval( count );
        if ( count ) {
            uint index = 0;
            for ( uint i = 0; i < ids.Count( ); i++ ) {
                auto itemData = static_cast<const CategoryTreeItem*>( this->GetItemData( ids[i] ) );
                if ( itemData->dataType( ) == CategoryTreeItem::DT_Entry ) {
                    retval[index++] = static_cast<const DatIndexEntry*>( itemData->data( ) );
                } else if ( itemData->dataType( ) == CategoryTreeItem::DT_Category ) {
                    this->addCategoryEntriesToArray( retval, index, *static_cast<const DatIndexCategory*>( itemData->data( ) ) );
                }
            }
            Assert( index == count );
        }

        return retval;
    }

    //============================================================================/

    wxTreeItemId CategoryTree::findEntry( const wxString& p_string ) {
        if ( !m_index ) {
            return wxTreeItemId( );
        }

        ulong baseId = 0;
        if ( !p_string.ToULong( &baseId ) ) {
            return wxTreeItemId( );
        }

        auto found = m_index->findEntryByBaseId( static_cast<uint32>( baseId ) );
        if ( !found ) {
            return wxTreeItemId( );
        }

        return this->revealEntry( *found );
    }

    //============================================================================/

    wxTreeItemId CategoryTree::navigateToEntry( const DatIndexEntry& p_entry ) {
        this->Freeze( );
        auto item = this->revealEntry( p_entry );
        this->Thaw( );

        if ( !item.IsOk( ) ) {
            return item;
        }

        this->UnselectAll( );
        this->SelectItem( item );
        this->ScrollTo( item );
        return item;
    }

    //============================================================================/

    wxTreeItemId CategoryTree::revealEntry( const DatIndexEntry& p_entry ) {
        auto category = p_entry.category( );
        if ( !category ) {
            return wxTreeItemId( );
        }

        // Build the chain of categories from the entry's category up to the root.
        std::vector<const DatIndexCategory*> chain;
        for ( auto current = category; current != nullptr; current = current->parent( ) ) {
            chain.push_back( current );
        }

        // Walk top-down, ensuring each category node exists. Expand ancestors so
        // the path is visible, but defer expanding the leaf until we know whether
        // we can insert only the target entry.
        wxTreeItemId categoryItem;
        for ( auto it = chain.rbegin( ); it != chain.rend( ); ++it ) {
            const bool isLeaf = ( it + 1 == chain.rend( ) );
            categoryItem = this->ensureHasCategory( **it, true );
            if ( !categoryItem.IsOk( ) ) {
                return wxTreeItemId( );
            }

            if ( !isLeaf && !this->IsExpanded( categoryItem ) ) {
                this->Expand( categoryItem );
            }
        }

        wxTreeItemId entryItem = this->findChildEntry( categoryItem, p_entry );

        if ( !entryItem.IsOk( ) ) {
            if ( this->IsExpanded( categoryItem ) ) {
                // Already expanded (e.g. a previous background fill is still
                // running) but our target isn't inserted yet. Insert it directly
                // so the find is instant.
                entryItem = this->addEntry( categoryItem, p_entry );
                this->SetItemData( entryItem, new CategoryTreeItem( CategoryTreeItem::DT_Entry, &p_entry ) );
            } else {
                // Expanding a dirty leaf category would populate every sibling
                // entry synchronously and freeze the UI. Insert only the target
                // entry via selective expand instead.
                m_selectiveExpandEntry = &p_entry;
                this->Expand( categoryItem );
                m_selectiveExpandEntry = nullptr;
                entryItem = this->findChildEntry( categoryItem, p_entry );
            }
        } else if ( !this->IsExpanded( categoryItem ) ) {
            m_selectiveExpandEntry = &p_entry;
            this->Expand( categoryItem );
            m_selectiveExpandEntry = nullptr;
        }

        // Load the rest of the siblings (and this category's subcategory folders)
        // in the background so they appear without blocking. The dirty flag is
        // cleared only after a full populate, so a clean category needs nothing.
        auto categoryData = static_cast<const CategoryTreeItem*>( this->GetItemData( categoryItem ) );
        const bool fullyPopulated = categoryData && !categoryData->isDirty( )
            && this->countEntryChildren( categoryItem ) >= category->numEntries( );
        if ( !fullyPopulated ) {
            this->beginBackgroundFill( categoryItem, *category );
        }

        return entryItem;
    }

    //============================================================================/

    void CategoryTree::beginBackgroundFill( const wxTreeItemId& p_item, const DatIndexCategory& p_category ) {
        // Restart any previous fill; we only track one at a time.
        this->cancelBackgroundFill( );

        m_bgFillItem = p_item;
        m_bgFillCategory = &p_category;
        m_bgFillNext = 0;
        m_bgFillPresent.clear( );

        // Record entries already present so we never insert duplicates.
        wxTreeItemIdValue cookie;
        auto child = this->GetFirstChild( p_item, cookie );
        while ( child.IsOk( ) ) {
            auto data = static_cast<const CategoryTreeItem*>( this->GetItemData( child ) );
            if ( data && data->dataType( ) == CategoryTreeItem::DT_Entry ) {
                m_bgFillPresent.insert( data->data( ) );
            }
            child = this->GetNextChild( p_item, cookie );
        }

        m_bgFillActive = true;
        this->Bind( wxEVT_IDLE, &CategoryTree::onIdleFill, this );

        for ( auto const& it : m_listeners ) {
            it->onTreeBackgroundLoadBegin( *this, p_category.numEntries( ) );
        }
    }

    //============================================================================/

    void CategoryTree::cancelBackgroundFill( ) {
        if ( !m_bgFillActive ) {
            return;
        }
        this->Unbind( wxEVT_IDLE, &CategoryTree::onIdleFill, this );
        m_bgFillActive = false;
        m_bgFillCategory = nullptr;
        m_bgFillItem = wxTreeItemId( );
        m_bgFillNext = 0;
        m_bgFillPresent.clear( );

        for ( auto const& it : m_listeners ) {
            it->onTreeBackgroundLoadEnd( *this );
        }
    }

    //============================================================================/

    void CategoryTree::onIdleFill( wxIdleEvent& p_event ) {
        if ( !m_bgFillActive || !m_bgFillCategory || !m_bgFillItem.IsOk( ) ) {
            this->cancelBackgroundFill( );
            return;
        }

        // Number of entries to add per idle tick. Small enough to keep the UI
        // responsive, large enough to finish quickly.
        const uint BATCH = 500;
        auto category = m_bgFillCategory;
        const uint total = category->numEntries( );

        this->Freeze( );
        uint added = 0;
        while ( m_bgFillNext < total && added < BATCH ) {
            auto entry = category->entry( m_bgFillNext++ );
            if ( !entry ) {
                continue;
            }
            if ( m_bgFillPresent.count( entry ) ) {
                continue;
            }
            this->AppendItem( m_bgFillItem, entry->name( ), this->getImageForEntry( *entry ), -1,
                new CategoryTreeItem( CategoryTreeItem::DT_Entry, entry ) );
            m_bgFillPresent.insert( entry );
            added++;
        }
        this->Thaw( );

        for ( auto const& it : m_listeners ) {
            it->onTreeBackgroundLoadUpdate( *this, m_bgFillNext, total );
        }

        if ( m_bgFillNext < total ) {
            // Keep getting idle events until we're done.
            p_event.RequestMore( );
            return;
        }

        // Finished: sort entries and append subcategories, matching a normal
        // expand. SortChildren preserves existing item ids (and the selection).
        this->Freeze( );
        this->SortChildren( m_bgFillItem );
        for ( uint i = 0; i < category->numSubCategories( ); i++ ) {
            auto subcategory = category->subCategory( i );
            if ( subcategory ) {
                this->ensureHasCategory( *subcategory, true );
            }
        }
        auto itemData = static_cast<CategoryTreeItem*>( this->GetItemData( m_bgFillItem ) );
        if ( itemData && itemData->dataType( ) == CategoryTreeItem::DT_Category ) {
            itemData->setDirty( false );
        }
        this->Thaw( );

        this->cancelBackgroundFill( );
    }

    //============================================================================/

    uint CategoryTree::countEntryChildren( const wxTreeItemId& p_parent ) const {
        uint count = 0;
        wxTreeItemIdValue cookie;
        auto child = this->GetFirstChild( p_parent, cookie );

        while ( child.IsOk( ) ) {
            auto data = static_cast<const CategoryTreeItem*>( this->GetItemData( child ) );
            if ( data && data->dataType( ) == CategoryTreeItem::DT_Entry ) {
                count++;
            }
            child = this->GetNextChild( p_parent, cookie );
        }

        return count;
    }

    //============================================================================/

    wxTreeItemId CategoryTree::findChildEntry( const wxTreeItemId& p_parent, const DatIndexEntry& p_entry ) const {
        wxTreeItemIdValue cookie;
        auto child = this->GetFirstChild( p_parent, cookie );

        while ( child.IsOk( ) ) {
            auto data = static_cast<const CategoryTreeItem*>( this->GetItemData( child ) );
            if ( data && data->dataType( ) == CategoryTreeItem::DT_Entry && data->data( ) == &p_entry ) {
                return child;
            }
            child = this->GetNextChild( p_parent, cookie );
        }

        return wxTreeItemId( );
    }

    //============================================================================/

    void CategoryTree::addCategoryEntriesToArray( Array<const DatIndexEntry*>& p_array, uint& p_index, const DatIndexCategory& p_category ) const {
        // Loop through subcategories
        for ( uint i = 0; i < p_category.numSubCategories( ); i++ ) {
            this->addCategoryEntriesToArray( p_array, p_index, *p_category.subCategory( i ) );
        }

        // Loop through entries
        for ( uint i = 0; i < p_category.numEntries( ); i++ ) {
            p_array[p_index++] = p_category.entry( i );
        }
    }

    //============================================================================/

    void CategoryTree::removeNonCategoryChildren( const wxTreeItemId& p_parent ) {
        wxTreeItemIdValue cookie;
        auto child = this->GetFirstChild( p_parent, cookie );

        while ( child.IsOk( ) ) {
            auto next = this->GetNextChild( p_parent, cookie );
            auto data = static_cast<CategoryTreeItem*>( this->GetItemData( child ) );

            if ( !data || data->dataType( ) != CategoryTreeItem::DT_Category ) {
                this->Delete( child );
            }

            child = next;
        }
    }

    //============================================================================/

    void CategoryTree::buildCategorySubtree( const DatIndexCategory& p_category ) {
        this->ensureHasCategory( p_category, true );

        for ( uint i = 0; i < p_category.numSubCategories( ); i++ ) {
            auto subCategory = p_category.subCategory( i );
            if ( subCategory ) {
                this->buildCategorySubtree( *subCategory );
            }
        }
    }

    //============================================================================/

    void CategoryTree::buildCategoryTreeFromIndex( ) {
        this->Freeze( );

        this->clearEntries( );

        if ( m_index ) {
            for ( uint i = 0; i < m_index->numCategories( ); i++ ) {
                auto category = m_index->category( i );
                if ( category && !category->parent( ) ) {
                    this->buildCategorySubtree( *category );
                }
            }
        }

        this->Thaw( );
    }

    //============================================================================/

    void CategoryTree::refreshAfterBulkUpdate( ) {
        this->buildCategoryTreeFromIndex( );
    }

    //============================================================================/

    void CategoryTree::setDatIndex( const std::shared_ptr<DatIndex>& p_index ) {
        if ( m_index ) {
            m_index->removeListener( this );
        }

        m_index = p_index;

        if ( m_index ) {
            m_index->addListener( this );

            if ( m_index->numEntries( ) > 500 ) {
                this->buildCategoryTreeFromIndex( );
            } else {
                this->clearEntries( );

                for ( uint i = 0; i < m_index->numEntries( ); i++ ) {
                    this->addEntry( *m_index->entry( i ) );
                }
            }
        } else {
            this->clearEntries( );
        }
    }

    //============================================================================/

    std::shared_ptr<DatIndex> CategoryTree::datIndex( ) const {
        return m_index;
    }

    //============================================================================/

    void CategoryTree::addListener( ICategoryTreeListener* p_listener ) {
        m_listeners.insert( p_listener );
    }

    //============================================================================/

    void CategoryTree::removeListener( ICategoryTreeListener* p_listener ) {
        m_listeners.erase( p_listener );
    }

    //============================================================================/

    int CategoryTree::getImageForEntry( const DatIndexEntry& p_entry ) {
        switch ( p_entry.fileType( ) ) {
        case ANFT_ATEX:
        case ANFT_ATTX:
        case ANFT_ATEC:
        case ANFT_ATEP:
        case ANFT_ATEU:
        case ANFT_ATET:
        case ANFT_CTEX:
        case ANFT_DDS:
        case ANFT_JPEG:
        case ANFT_WEBP:
        case ANFT_PNG:
            return CategoryTreeImageList::IT_Image;
        case ANFT_EXE:
            return CategoryTreeImageList::IT_Executable;
        case ANFT_DLL:
            return CategoryTreeImageList::IT_Dll;
        case ANFT_EULA:
        case ANFT_StringFile:
        case ANFT_TEXT:
        case ANFT_UTF8:
            return CategoryTreeImageList::IT_Text;
        case ANFT_Bank:
        case ANFT_Sound:
        case ANFT_Ogg:
        case ANFT_MP3:
        case ANFT_asndMP3:
        case ANFT_asndOgg:
        case ANFT_PackedMP3:
        case ANFT_PackedOgg:
            return CategoryTreeImageList::IT_Sound;
        case ANFT_FontFile:
            return CategoryTreeImageList::IT_Font;
        case ANFT_BitmapFontFile:
            return CategoryTreeImageList::IT_BitmapFont;
        case ANFT_Bink2Video:
            return CategoryTreeImageList::IT_Video;
        case ANFT_Model:
            return CategoryTreeImageList::IT_Model;
        default:
            return CategoryTreeImageList::IT_UnknownFile;
        }
    }

    //============================================================================/

    void CategoryTree::onItemExpanding( wxTreeEvent& p_event ) {
        auto id = p_event.GetItem( );
        auto itemData = static_cast<CategoryTreeItem*>( this->GetItemData( id ) );

        // If this is not a category, skip it
        if ( !itemData || itemData->dataType( ) != CategoryTreeItem::DT_Category ) {
            return;
        }

        // Give it the open folder icon instead
        this->SetItemImage( id, CategoryTreeImageList::IT_OpenFolder );

        // Fetch the category info
        auto category = static_cast<const DatIndexCategory*>( itemData->data( ) );
        if ( !category ) {
            return;
        }

        // Selective expand: only insert the entry being searched for. Remaining
        // entries and subcategories are added later by the background fill, which
        // preserves the normal "sorted entries, then folders" ordering.
        if ( m_selectiveExpandEntry && m_selectiveExpandEntry->category( ) == category ) {
            this->removeNonCategoryChildren( id );

            auto node = this->addEntry( id, *m_selectiveExpandEntry );
            this->SetItemData( node, new CategoryTreeItem( CategoryTreeItem::DT_Entry, m_selectiveExpandEntry ) );

            // Leave dirty: the category is only partially populated for now.
            return;
        }

        // A category revealed by find may contain only the matched entry. If the
        // user expands it later, load the remaining entries then.
        if ( !itemData->isDirty( ) ) {
            if ( this->countEntryChildren( id ) < category->numEntries( ) ) {
                itemData->setDirty( true );
            } else {
                return;
            }
        }

        // If a background fill is populating this same category, cancel it; we
        // are about to repopulate it fully and would otherwise insert duplicates.
        if ( m_bgFillActive && m_bgFillItem == id ) {
            this->cancelBackgroundFill( );
        }

        // Remove stale file entries and wx placeholder nodes, keep subcategory folders
        this->removeNonCategoryChildren( id );

        // Add all contained entries
        for ( uint i = 0; i < category->numEntries( ); i++ ) {
            auto entry = category->entry( i );
            if ( !entry ) {
                continue;
            }
            //this->AddEntry(id, *entry);
            this->AppendItem( id, entry->name( ), this->getImageForEntry( *entry ), -1, new CategoryTreeItem( CategoryTreeItem::DT_Entry, entry ) );
        }
        this->SortChildren( id );

        // Add sub-categories last
        for ( uint i = 0; i < category->numSubCategories( ); i++ ) {
            auto subcategory = category->subCategory( i );
            if ( !subcategory ) {
                continue;
            }
            this->ensureHasCategory( *subcategory, true );
        }

        // Un-dirty!
        itemData->setDirty( false );
    }

    //============================================================================/

    void CategoryTree::onItemCollapsing( wxTreeEvent& p_event ) {
        auto id = p_event.GetItem( );
        auto* itemData = static_cast<const CategoryTreeItem*>( this->GetItemData( id ) );

        // Skip if this is not a category
        if ( !itemData || itemData->dataType( ) != CategoryTreeItem::DT_Category ) {
            return;
        }

        // Set icon to the closed folder
        this->SetItemImage( id, CategoryTreeImageList::IT_ClosedFolder );
    }

    //============================================================================/

    void CategoryTree::onSelChanged( wxTreeEvent& p_event ) {
        wxArrayTreeItemIds ids;
        this->GetSelections( ids );

        // Only raise events if only one entry was selected
        if ( ids.Count( ) == 1 ) {
            auto itemData = static_cast<const CategoryTreeItem*>( this->GetItemData( ids[0] ) );
            if ( !itemData ) {
                return;
            }

            // raise the correct event
            switch ( itemData->dataType( ) ) {
            case CategoryTreeItem::DT_Category:
                for ( auto const& it : m_listeners ) {
                    it->onTreeCategoryClicked( *this, *static_cast<const DatIndexCategory*>( itemData->data( ) ) );
                }
                break;
            case CategoryTreeItem::DT_Entry:
                for ( auto const& it : m_listeners ) {
                    it->onTreeEntryClicked( *this, *static_cast<const DatIndexEntry*>( itemData->data( ) ) );
                }
            }
        }
    }

    //============================================================================/

    void CategoryTree::onContextMenu( wxTreeEvent& p_event ) {
        wxArrayTreeItemIds ids;
        this->GetSelections( ids );

        if ( ids.Count( ) > 0 ) {
            // Start with counting the total amount of entries
            uint count = 0;
            const DatIndexEntry* firstEntry = nullptr;
            for ( uint i = 0; i < ids.Count( ); i++ ) {
                auto itemData = static_cast<const CategoryTreeItem*>( this->GetItemData( ids[i] ) );
                if ( itemData->dataType( ) == CategoryTreeItem::DT_Entry ) {
                    if ( !firstEntry ) {
                        firstEntry = static_cast<const DatIndexEntry*>( itemData->data( ) );
                    }
                    count++;
                } else if ( itemData->dataType( ) == CategoryTreeItem::DT_Category ) {
                    auto category = static_cast<const DatIndexCategory*>( itemData->data( ) );
                    count += category->numEntries( true );
                    if ( !firstEntry && category->numEntries( ) ) {
                        firstEntry = category->entry( 0 );
                    }
                }
            }

            // Create the menu
            if ( count > 0 ) {
                wxMenu newMenu;
                if ( count == 1 ) {
                    newMenu.Append( wxID_SAVE, wxString::Format( wxT( "Extract file %s..." ), firstEntry->name( ) ) );
                    newMenu.Append( wxID_SAVEAS, wxString::Format( wxT( "Extract file %s (raw)..." ), firstEntry->name( ) ) );
                } else {
                    newMenu.Append( wxID_SAVE, wxString::Format( wxT( "Extract %d files..." ), count ) );
                    newMenu.Append( wxID_SAVEAS, wxString::Format( wxT( "Extract %d files (raw)..." ), count ) );
                }
                this->PopupMenu( &newMenu );
            }
        }
    }

    //============================================================================/

    void CategoryTree::onExtractRawFiles( wxCommandEvent& p_event ) {
        for ( auto const& it : m_listeners ) {
            it->onTreeExtractFile( *this, Exporter::EM_Raw );
        }
    }

    //============================================================================/

    void CategoryTree::onExtractConvertedFiles( wxCommandEvent& p_event ) {
        for ( auto const& it : m_listeners ) {
            it->onTreeExtractFile( *this, Exporter::EM_Converted );
        }
    }

    //============================================================================/

    void CategoryTree::onIndexFileAdded( DatIndex& p_index, const DatIndexEntry& p_entry ) {
        Ensure::notNull( &p_entry );
        this->addEntry( p_entry );
    }

    //============================================================================/

    void CategoryTree::onIndexCleared( DatIndex& p_index ) {
        Assert( &p_index == m_index.get( ) );
        this->clearEntries( );
    }

    //============================================================================/

    void CategoryTree::onIndexDestruction( DatIndex& p_index ) {
        Assert( &p_index == m_index.get( ) );
        m_index = nullptr;
        this->clearEntries( );
    }

}; // namespace gw2b
