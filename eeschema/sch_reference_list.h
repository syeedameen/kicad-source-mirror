/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2011 jean-pierre Charras <jean-pierre.charras@gipsa-lab.inpg.fr>
 * Copyright (C) 1992-2011 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2020 KiCad Developers, see authors.txt for contributors.
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

#ifndef _SCH_REFERENCE_LIST_H_
#define _SCH_REFERENCE_LIST_H_


#include <macros.h>

#include <class_libentry.h>
#include <sch_sheet_path.h>
#include <sch_component.h>
#include <sch_text.h>

#include <map>

/**
 * SCH_REFERENCE
 * is used as a helper to define a symbol's reference designator in a schematic.  This helper
 * is required in a complex hierarchy because a symbol can be used more than once and its
 * reference depends on the sheet path.  This class is used to flatten the schematic hierarchy
 * for annotation, net list generation, and bill of material generation.
 */
class SCH_REFERENCE
{
    /// Symbol reference prefix, without number (for IC1, this is IC) )
    UTF8            m_ref;               // it's private, use the accessors please
    SCH_COMPONENT*  m_rootSymbol;        ///< The symbol associated the reference object.
    LIB_PART*       m_libPart;           ///< The source symbol from a library.
    wxPoint         m_symbolPos;         ///< The physical position of the symbol in schematic
                                        ///< used to annotate by X or Y position
    int             m_unit;              ///< The unit number for symbol with multiple parts
                                        ///< per package.
    wxString        m_value;             ///< The symbol value.
    wxString        m_footprint;         ///< The footprint assigned.
    SCH_SHEET_PATH  m_sheetPath;         ///< The sheet path for this reference.
    bool            m_isNew;             ///< True if not yet annotated.
    int             m_sheetNum;          ///< The sheet number for the reference.
    KIID            m_symbolUuid;        ///< UUID of the symbol.
    int             m_numRef;            ///< The numeric part of the reference designator.
    int             m_flag;

    friend class SCH_REFERENCE_LIST;

public:

    SCH_REFERENCE() :
            m_sheetPath()
    {
        m_rootSymbol      = NULL;
        m_libPart         = NULL;
        m_unit            = 0;
        m_isNew           = false;
        m_numRef          = 0;
        m_flag            = 0;
        m_sheetNum        = 0;
    }

    SCH_REFERENCE( SCH_COMPONENT* aSymbol, LIB_PART* aLibPart, const SCH_SHEET_PATH& aSheetPath );

    SCH_COMPONENT* GetSymbol() const           { return m_rootSymbol; }

    LIB_PART*      GetLibPart() const          { return m_libPart; }

    const SCH_SHEET_PATH& GetSheetPath() const { return m_sheetPath; }

    SCH_SHEET_PATH& GetSheetPath()             { return m_sheetPath; }

    int GetUnit() const                        { return m_unit; }
    void SetUnit( int aUnit )                  { m_unit = aUnit; }

    const wxString GetValue() const            { return m_value; }
    void SetValue( const wxString& aValue )    { m_value = aValue; }

    const wxString GetFootprint() const        { return m_footprint; }
    void SetFootprint( const wxString& aFP )   { m_footprint = aFP; }

    void SetSheetNumber( int aSheetNumber )    { m_sheetNum = aSheetNumber; }

    const wxString GetPath() const
    {
        return m_rootSymbol ? m_sheetPath.PathAsString() + m_rootSymbol->m_Uuid.AsString() : "";
    }

    /**
     * Function Annotate
     * updates the annotation of the symbol according the the current object state.
     */
    void Annotate();

    /**
     * Function Split
     * attempts to split the reference designator into a name (U) and number (1).  If the
     * last character is '?' or not a digit, the reference is tagged as not annotated.  For
     * sybmols with multiple parts per package that are not already annotated, sets m_unit to
     * a max value (0x7FFFFFFF).
     */
    void Split();

    void SetRef( const wxString& aReference ) { m_ref = aReference; }
    wxString GetRef() const { return m_ref; }

    void SetRefStr( const std::string& aReference ) { m_ref = aReference; }
    const char* GetRefStr() const { return m_ref.c_str(); }

    ///> Return reference name with unit altogether
    wxString GetFullRef()
    {
        if( GetSymbol()->GetUnitCount() > 1 )
            return GetRef() + LIB_PART::SubReference( GetUnit() );
        else
            return GetRef();
    }

    wxString GetRefNumber() const
    {
        wxString ref;

        if( m_numRef < 0 )
            return wxT( "?" );

        // To avoid a risk of duplicate, for power symbols the ref number is 0nnn instead of nnn.
        // Just because sometimes only power symbols are annotated
        if( GetLibPart() && GetLibPart()->IsPower() )
            ref = wxT( "0" );

        return ref << m_numRef;
    }

    int CompareValue( const SCH_REFERENCE& item ) const
    {
        return m_value.Cmp( item.m_value );
    }

    int CompareRef( const SCH_REFERENCE& item ) const
    {
        return m_ref.compare( item.m_ref );
    }

    int CompareLibName( const SCH_REFERENCE& item ) const
    {
        return m_rootSymbol->GetLibId().GetLibItemName().compare(
            item.m_rootSymbol->GetLibId().GetLibItemName() );
    }

    /**
     * Function IsSameInstance
     * returns whether this reference refers to the same symbol instance (symbol and sheet) as
     * another.
     */
    bool IsSameInstance( const SCH_REFERENCE& other ) const
    {
        // JEY TODO: should this be checking unit as well?
        return GetSymbol() == other.GetSymbol()
               && GetSheetPath().Path() == other.GetSheetPath().Path();
    }

    bool IsUnitsLocked()
    {
        return m_libPart->UnitsLocked();
    }
};


/**
 * SCH_REFERENCE_LIST
 * is used to create a flattened list of symbols because in a complex hierarchy, a symbol
 * can be used more than once and its reference designator is dependent on the sheet path for
 * the same symbol.  This flattened list is used for netlist generation, BOM generation, and
 * schematic annotation.
 */
class SCH_REFERENCE_LIST
{
private:
    std::vector<SCH_REFERENCE> flatList;

public:
    /** Constructor
     */
    SCH_REFERENCE_LIST()
    {
    }

    SCH_REFERENCE& operator[]( int aIndex )
    {
        return flatList[ aIndex ];
    }

    void Clear()
    {
        flatList.clear();
    }

    unsigned GetCount() const { return flatList.size(); }

    SCH_REFERENCE& GetItem( int aIdx ) { return flatList[aIdx]; }

    void AddItem( SCH_REFERENCE& aItem ) { flatList.push_back( aItem ); }

    /**
     * Function RemoveItem
     * removes an item from the list of references.
     *
     * @param aIndex is the index of the item to be removed.
     */
    void RemoveItem( unsigned int aIndex );

    /* Sort functions:
     * Sort functions are used to sort symbols for annotation or BOM generation.  Because
     * sorting depends on what we want to do, there are many sort functions.
     * Note:
     *    When creating BOM, symbols are fully annotated.  References are something like U3,
     *    U5 or R4, R8.  When annotating,  some or all suymbols are not annotated, i.e. ref is
     *    only U or R, with no number.
     */

    /**
     * Function SplitReferences
     * attempts to split all reference designators into a name (U) and number (1).  If the
     * last character is '?' or not a digit, the reference is tagged as not annotated.  For
     * symbols with multiple parts per package that are not already annotated, set m_unit to
     * a max value (0x7FFFFFFF).
     * @see SCH_REFERENCE::Split()
     */
    void SplitReferences()
    {
        for( unsigned ii = 0; ii < GetCount(); ii++ )
            flatList[ii].Split();
    }

    /**
     * function UpdateAnnotation
     * Updates the symbol references for the schematic project (or the current sheet).
     * Note: this function does not calculate the reference numbers stored in m_numRef so it
     * must be called after calculation of new reference numbers
     * @see SCH_REFERENCE::Annotate()
     */
    void UpdateAnnotation()
    {
        /* update the reference numbers */
        for( unsigned ii = 0; ii < GetCount(); ii++ )
            flatList[ii].Annotate();
    }

    /**
     * Function Annotate
     * set the reference designators in the list that have not been annotated.
     * @param aUseSheetNum Set to true to start annotation for each sheet at the sheet number
     *                     times \a aSheetIntervalId.  Otherwise annotate incrementally.
     * @param aSheetIntervalId The per sheet reference designator multiplier.
     * @param aStartNumber The number to start with if NOT numbering based on sheet number.
     * @param aLockedUnitMap A SCH_MULTI_UNIT_REFERENCE_MAP of reference designator wxStrings
     *      to SCH_REFERENCE_LISTs. May be an empty map. If not empty, any multi-unit parts
     *      found in this map will be annotated as a group rather than individually.
     * <p>
     * If a the sheet number is 2 and \a aSheetIntervalId is 100, then the first reference
     * designator would be 201 and the last reference designator would be 299 when no overlap
     * occurs with sheet number 3.  If there are 150 items in sheet number 2, then items are
     * referenced U201 to U351, and items in sheet 3 start from U352
     * </p>
     */
    void Annotate( bool aUseSheetNum, int aSheetIntervalId, int aStartNumber,
                   SCH_MULTI_UNIT_REFERENCE_MAP aLockedUnitMap );

    /**
     * Function CheckAnnotation
     * check for annotations errors.
     * <p>
     * The following annotation error conditions are tested:
     * <ul>
     * <li>Symbols not annotated.</li>
     * <li>Symbols having the same reference designator (duplicates).</li>
     * <li>Symbols with multiple parts per package having different reference designators.</li>
     * <li>Symbols with multiple parts per package with invalid part count.</li>
     * </ul>
     * </p>
     * @param aReporter A sink for error messages.  Use NULL_REPORTER if you don't need errors.
     * @return The number of errors found.
     */
    int CheckAnnotation( REPORTER& aReporter );

    /**
     * Function SortByXCoordinate
     * sorts the list of references by X position.
     * <p>
     * Symbols are sorted as follows:
     * <ul>
     * <li>Numeric value of reference designator.</li>
     * <li>Sheet number.</li>
     * <li>X coordinate position.</li>
     * <li>Y coordinate position.</li>
     * <li>Time stamp.</li>
     * </ul>
     * </p>
     */
    void SortByXCoordinate()
    {
        sort( flatList.begin(), flatList.end(), sortByXPosition );
    }

    /**
     * Function SortByYCoordinate
     * sorts the list of references by Y position.
     * <p>
     * Symbols are sorted as follows:
     * <ul>
     * <li>Numeric value of reference designator.</li>
     * <li>Sheet number.</li>
     * <li>Y coordinate position.</li>
     * <li>X coordinate position.</li>
     * <li>Time stamp.</li>
     * </ul>
     * </p>
     */
    void SortByYCoordinate()
    {
        sort( flatList.begin(), flatList.end(), sortByYPosition );
    }

    /**
     * Function SortByTimeStamp
     * sort the flat list by Time Stamp (sheet path + timestamp).
     * Useful to detect duplicate Time Stamps
     */
    void SortByTimeStamp()
    {
        sort( flatList.begin(), flatList.end(), sortByTimeStamp );
    }

    /**
     * Function SortByRefAndValue
     * sorts the list of references by value.
     * <p>
     * Symbols are sorted in the following order:
     * <ul>
     * <li>Numeric value of reference designator.</li>
     * <li>Value of symbol.</li>
     * <li>Unit number when symbol has multiple parts.</li>
     * <li>Sheet number.</li>
     * <li>X coordinate position.</li>
     * <li>Y coordinate position.</li>
     * </ul>
     * </p>
     */
    void SortByRefAndValue()
    {
        sort( flatList.begin(), flatList.end(), sortByRefAndValue );
    }

    /**
     * Function SortByReferenceOnly
     * sorts the list of references by reference.
     * <p>
     * Symbols are sorted in the following order:
     * <ul>
     * <li>Numeric value of reference designator.</li>
     * <li>Unit number when symbol has multiple parts.</li>
     * </ul>
     * </p>
     */
    void SortByReferenceOnly()
    {
        sort( flatList.begin(), flatList.end(), sortByReferenceOnly );
    }

    /**
     * searches the list for a symbol with a given reference.
     * @param aPath
     * @return
     */
    int FindRef( const wxString& aPath ) const;

    /**
     * searches the sorted list of symbols for a another symbol with the same reference and a
     * given part unit.  Use this method to manage symbols with multiple parts per package.
     * @param aIndex = index in aSymbolsList for of given SCH_REFERENCE item to test.
     * @param aUnit = the given unit number to search
     * @return index in aSymbolsList if found or -1 if not found
     */
    int FindUnit( size_t aIndex, int aUnit );

    /**
     * searches the list for a symbol with the given KIID path
     * @param aPath path to search
     * @return index in aSymbolsList if found or -1 if not found
     */
    int FindRefByPath( const wxString& aPath ) const;

    /**
     * Function GetRefsInUse
     * adds all the reference designator numbers greater than \a aMinRefId to \a aIdList
     * skipping the reference at \a aIndex.
     * @param aIndex = the current symbol's index to use for reference prefix filtering.
     * @param aIdList = the buffer to fill
     * @param aMinRefId = the min id value to store. all values < aMinRefId are ignored
     */
    void GetRefsInUse( int aIndex, std::vector< int >& aIdList, int aMinRefId );

    /**
     * Function GetLastReference
     * returns the last used (greatest) reference number in the reference list for the prefix
     * used by the symbol pointed to by \a aIndex.  The symbol list must be sorted.
     *
     * @param aIndex The index of the reference item used for the search pattern.
     * @param aMinValue The minimum value for the current search.
     */
    int GetLastReference( int aIndex, int aMinValue );

#if defined(DEBUG)
    void Show( const char* aPrefix = "" )
    {
        printf( "%s\n", aPrefix );

        for( unsigned i=0; i < flatList.size(); ++i )
        {
            SCH_REFERENCE& schref = flatList[i];

            printf( " [%-2d] ref:%-8s num:%-3d lib_part:%s\n",
                    i,
                    schref.m_ref.c_str(),
                    schref.m_numRef,
                    TO_UTF8( schref.GetLibPart()->GetName() ) );
        }
    }
#endif

    /**
     * Function Shorthand
     * Returns a shorthand string representing all the references in the list.  For instance,
     * "R1, R2, R4 - R7, U1"
     * @param aList
     */
    static wxString Shorthand( std::vector<SCH_REFERENCE> aList );

    friend class BACK_ANNOTATION;

private:
    /* sort functions used to sort flatList
    */

    static bool sortByRefAndValue( const SCH_REFERENCE& item1, const SCH_REFERENCE& item2 );

    static bool sortByXPosition( const SCH_REFERENCE& item1, const SCH_REFERENCE& item2 );

    static bool sortByYPosition( const SCH_REFERENCE& item1, const SCH_REFERENCE& item2 );

    static bool sortByTimeStamp( const SCH_REFERENCE& item1, const SCH_REFERENCE& item2 );

    static bool sortByReferenceOnly( const SCH_REFERENCE& item1, const SCH_REFERENCE& item2 );

    /**
     * Function CreateFirstFreeRefId
     * searches for the first free reference number in \a aListId of reference numbers in use.
     * This function just searches for a hole in a list of incremented numbers, this list must
     * be sorted by increasing values and each value can be stored only once.  The new value
     * is added to the list.
     * @see BuildRefIdInUseList to prepare this list
     * @param aIdList The buffer that contains the reference numbers in use.
     * @param aFirstValue The first expected free value
     * @return The first free (not yet used) value.
     */
    int CreateFirstFreeRefId( std::vector<int>& aIdList, int aFirstValue );

    // Used for sorting static sortByTimeStamp function
    friend class BACK_ANNOTATE;
};

#endif    // _SCH_REFERENCE_LIST_H_
