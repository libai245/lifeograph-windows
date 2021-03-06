/***********************************************************************************

    Copyright (C) 2007-2011 Ahmet Öztürk (aoz_2@yahoo.com)

    This file is part of Lifeograph.

    Lifeograph is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Lifeograph is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Lifeograph.  If not, see <http://www.gnu.org/licenses/>.

***********************************************************************************/


#ifndef LIFEOGRAPH_ENTRY_HEADER
#define LIFEOGRAPH_ENTRY_HEADER


#include "diarydata.hpp"


namespace LIFEO
{

static const char LANG_INHERIT_DIARY[] = "d";


class Entry : virtual public DiaryElement
{
    public:
        static ElementView< Entry >* shower;

#ifndef LIFEO_WINDOZE
        typedef sigc::signal< void, Entry* >
                                Signal_void_Entry;
#endif

                                Entry( Diary* const, const Date::date_t,
                                       const Ustring&, bool = false );
                                Entry( Diary* const, const Date::date_t,
                                       bool = false );
        virtual                 ~Entry() {}

        void                    show() override;
        void                    prepare_for_hiding() override;

        const Ustring*          get_text() const { return &m_text; }
        Ustring&                get_text() { return m_text; }
        void                    set_text( const Ustring& text )
        { m_text = text; calculate_title( text ); }
        void                    append_text( const Ustring& text );

        Date                    get_date() const override { return m_date; }
        void                    set_date( Date::date_t date )
        { m_date.m_date = date; }

        time_t                  get_date_created() const { return m_date_created; }
        Ustring                 get_date_created_str() const;

        time_t                  get_date_edited() const { return m_date_edited; }
        Ustring                 get_date_edited_str() const;
        void                    set_date_edited( Date::date_t d ) { m_date_edited = d; }

        time_t                  get_date_status() const { return m_date_status; }
        Ustring                 get_date_status_str() const;
        void                    set_date_status( Date::date_t d ) { m_date_status = d; }

        int                     get_size() const override
        { return m_text.size(); }
        Type                    get_type() const override
        { return ET_ENTRY; }

#ifndef LIFEO_WINDOZE
        const Icon&             get_icon() const override;
        const Icon&             get_icon32() const override;
#else
        int                     get_icon() const;
#endif

        void                    calculate_title( const Ustring& );

        Ustring                 get_list_str() const override;

        bool                    get_filtered_out() override;
        void                    set_filtered_out( bool filteredout = true )
        {
            if( filteredout )
                m_status |= ES::FILTERED_OUT;
            else if( m_status & ES::FILTERED_OUT )
                m_status -= ES::FILTERED_OUT;
        }

        bool                    is_favored() const override { return( m_status & ES::FAVORED ); }
        void                    set_favored( bool favored )
        {
            m_status -= ( m_status & ES::FILTER_FAVORED );
            m_status |= ( favored ? ES::FAVORED : ES::NOT_FAVORED );
        }
        void                    toggle_favored()
        { m_status ^= ES::FILTER_FAVORED; }

        std::string             get_lang() const { return m_option_lang; }
        std::string             get_lang_final() const;
        void                    set_lang( const std::string& lang ) { m_option_lang = lang; }

        bool                    is_empty() const
        {
            return( m_text.empty() && m_tags.empty() );
        }

        bool                    is_trashed() const { return( m_status & ES::TRASHED ); }
        void                    set_trashed( bool trashed )
        {
            m_status -= ( m_status & ES::FILTER_TRASHED );
            m_status |= ( trashed ? ES::TRASHED : ES::NOT_TRASHED );
        }

        void                    set_tags( const TagSet& tags )
        { m_tags = tags; }
        const TagSet&           get_tags() const
        { return m_tags; }
        TagSet&                 get_tags()
        { return m_tags; }
        bool                    add_tag( Tag*, Value = 1.0 );
        bool                    remove_tag( Tag* );
        bool                    clear_tags();

        // THEMES
        const Theme*            get_theme() const;
        const Tag*              get_theme_tag() const
        { return m_ptr2theme_tag; }
        void                    set_theme_tag( const Tag* );
        bool                    get_theme_is_set() const
        { return( m_ptr2theme_tag != NULL ); }
        void                    update_theme(); // called when a tag gained or lost custom theme

    protected:
        Date                    m_date;
        time_t                  m_date_created;
        time_t                  m_date_edited;
        time_t                  m_date_status;
        Ustring                 m_text;
        TagSet                  m_tags;
        Ustring                 m_location;
        const Tag*              m_ptr2theme_tag{ nullptr }; // nullptr means theme is not set

        std::string             m_option_lang = LANG_INHERIT_DIARY;  // empty means off

    friend class Diary; // TODO: remove this friendship too??
};

// ENTRY SET ===================================================================
class PoolEntries : public std::map< Date::date_t, Entry*, FuncCompareDates >
{
    public:
                                PoolEntries()
    :   std::map< Date::date_t, Entry*, FuncCompareDates >( compare_dates ) {}
                                ~PoolEntries();

        void                    clear();
};

typedef PoolEntries::iterator               EntryIter;
typedef PoolEntries::reverse_iterator       EntryIterReverse;
typedef PoolEntries::const_iterator         EntryIterConst;
typedef PoolEntries::const_reverse_iterator EntryIterConstRev;

} // end of namespace LIFEO

#endif

