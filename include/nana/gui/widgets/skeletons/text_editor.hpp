/*
 *	A text editor implementation
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2017 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0. 
 *	(See accompanying file LICENSE_1_0.txt or copy at 
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/gui/widgets/skeletons/text_editor.hpp
 *	@description: 
 */

#ifndef NANA_GUI_SKELETONS_TEXT_EDITOR_HPP
#define NANA_GUI_SKELETONS_TEXT_EDITOR_HPP
#include <nana/push_ignore_diagnostic>

#include "textbase.hpp"
#include "text_editor_part.hpp"
#include <nana/unicode_bidi.hpp>

//#include <nana/paint/graphics.hpp>
#include <nana/gui/detail/general_events.hpp>

#include <functional>

namespace nana
{
	namespace paint
	{
		// Forward declaration
		class graphics;
	}
}

namespace nana{	namespace widgets
{
	namespace skeletons
	{
		class text_editor
		{
			struct attributes;
			class editor_behavior_interface;
			class behavior_normal;
			class behavior_linewrapped;

			enum class command{
				backspace, input_text, move_text,
			};
			//Commands for undoable
			template<typename EnumCommand> class basic_undoable;
			class undo_backspace;
			class undo_input_text;
			class undo_move_text;

			class keyword_parser;
			class helper_pencil;

			struct text_section;

			text_editor(const text_editor&) = delete;
			text_editor& operator=(const text_editor&) = delete;

			text_editor(text_editor&&) = delete;
			text_editor& operator=(text_editor&&) = delete;
		public:
			using char_type = wchar_t;
			using size_type = textbase<char_type>::size_type;
			using string_type = textbase<char_type>::string_type;

			using event_interface = text_editor_event_interface;

			using graph_reference = ::nana::paint::graphics&;

			struct renderers
			{
				std::function<void(graph_reference, const nana::rectangle& text_area, const ::nana::color&)> background;	///< a customized background renderer
				std::function<void(graph_reference, const ::nana::color&)> border;											///< a customized border renderer
			};

			enum class accepts
			{
				no_restrict, integer, real
			};

			text_editor(window, graph_reference, const text_editor_scheme*);
			~text_editor();

			void set_highlight(const ::std::string& name, const ::nana::color&, const ::nana::color&);
			void erase_highlight(const ::std::string& name);
			void set_keyword(const ::std::wstring& kw, const std::string& name, bool case_sensitive, bool whole_word_matched);
			void erase_keyword(const ::std::wstring& kw);

			void set_accept(std::function<bool(char_type)>);
			void set_accept(accepts);
			bool respond_char(const arg_keyboard& arg);
			bool respond_key(const arg_keyboard& arg);

			void typeface_changed();

			void indent(bool, std::function<std::string()> generator);
			void set_event(event_interface*);

			bool load(const char*);

			void text_align(::nana::align alignment);

			/// Sets the text area.
			/// @return true if the area is changed with the new value.
			bool text_area(const nana::rectangle&);

			/// Returns the text area
			rectangle text_area(bool including_scroll) const;

			bool tip_string(::std::string&&);

			/// Returns the reference of listbox attributes
			const attributes & attr() const noexcept;

			/// Set the text_editor whether it is line wrapped, it returns false if the state is not changed.
			bool line_wrapped(bool);

			bool multi_lines(bool);

			/// Enables/disables the editability of text_editor
			/**
			 * @param enable Indicates whether to enable or disable the editability
			 * @param enable_cart Indicates whether to show or hide the caret when the text_editor is not editable. It is ignored if enable is false.
			 */
			void editable(bool enable, bool enable_caret);
			void enable_background(bool);
			void enable_background_counterpart(bool);

			void undo_enabled(bool);
			bool undo_enabled() const;
			void undo_max_steps(std::size_t);
			std::size_t undo_max_steps() const;

			renderers& customized_renderers();
			void clear_undo();	///< same with undo_max_steps(0)

			unsigned line_height() const;
			unsigned screen_lines() const;

			bool getline(std::size_t pos, ::std::wstring&) const;
			void text(std::wstring, bool end_caret);
			std::wstring text() const;

			/// Sets caret position through text coordinate.
			/**
			 * @param pos the text position
			 * @param reset indicates whether to reset the text position by the pos. If this parameter is true, the text position is set by pos. If the parameter is false, it only moves the UI caret to the specified position. 
			 */
			bool move_caret(const upoint& pos, bool reset = false);
			void move_caret_end(bool update);
			void reset_caret_pixels() const;
			void reset_caret();
			void show_caret(bool isshow);

			bool selected() const;
			bool get_selected_points(nana::upoint&, nana::upoint&) const;

			bool select(bool);

			/// Sets the end position of a selected string.
			void set_end_caret();
			
			bool hit_text_area(const point&) const;
			bool hit_select_area(nana::upoint pos, bool ignore_when_select_all) const;

			bool move_select();
			bool mask(wchar_t);

			/// Returns width of text area excluding the vscroll size.
			unsigned width_pixels() const;
			window window_handle() const;

			/// Returns text position of each line that currently displays on screen
			const std::vector<upoint>& text_position() const;

			void focus_behavior(text_focus_behavior);
			void select_behavior(bool move_to_end);
		public:
			void draw_corner();
			void render(bool focused);
		public:
			void put(std::wstring);
			void put(wchar_t);
			void copy() const;
			void cut();
			void paste();
			void enter(bool record_undo = true);
			void del();
			void backspace(bool record_undo = true);
			void undo(bool reverse);
			void set_undo_queue_length(std::size_t len);
			void move_ns(bool to_north);	//Moves up and down
			void move_left();
			void move_right();
			const upoint& mouse_caret(const point& screen_pos);
			const upoint& caret() const;
			point caret_screen_pos() const;
			bool scroll(bool upwards, bool vertical);

			bool focus_changed(const arg_focus&);
			bool mouse_enter(bool entering);
			bool mouse_move(bool left_button, const point& screen_pos);
			bool mouse_pressed(const arg_mouse& arg);

			skeletons::textbase<char_type>& textbase();
			const skeletons::textbase<char_type>& textbase() const;
		private:
			std::vector<upoint> _m_render_text(const ::nana::color& text_color);
			void _m_pre_calc_lines(std::size_t line_off, std::size_t lines);

			::nana::point	_m_caret_to_screen(::nana::upoint pos) const;
			::nana::upoint	_m_screen_to_caret(::nana::point pos) const;

			bool _m_pos_from_secondary(std::size_t textline, const nana::upoint& secondary, unsigned & pos);
			bool _m_pos_secondary(const nana::upoint& charpos, nana::upoint& secondary_pos) const;
			bool _m_move_caret_ns(bool to_north);
			void _m_update_line(std::size_t pos, std::size_t secondary_count_before);

			bool _m_accepts(char_type) const;
			::nana::color _m_bgcolor() const;
			bool _m_scroll_text(bool vertical);
			void _m_scrollbar();

			::nana::rectangle _m_text_area() const;

			void _m_get_scrollbar_size();
			void _m_reset();
			::nana::upoint _m_put(::std::wstring);
			::nana::upoint _m_erase_select();

			::std::wstring _m_make_select_string() const;
			static bool _m_resolve_text(const ::std::wstring&, std::vector<std::pair<std::size_t, std::size_t>> & lines);

			bool _m_cancel_select(int align);
			unsigned _m_tabs_pixels(size_type tabs) const;
			nana::size _m_text_extent_size(const char_type*, size_type n) const;

			/// Moves the view of window.
			bool _m_move_offset_x_while_over_border(int many);
			bool _m_move_select(bool record_undo);

			int _m_text_top_base() const;

			/// Returns the logical position that text starts of a specified line in x-axis
			int _m_text_x(const text_section&) const;

			void _m_draw_parse_string(const keyword_parser&, bool rtl, ::nana::point pos, const ::nana::color& fgcolor, const wchar_t*, std::size_t len) const;
			//_m_draw_string
			//@brief: Draw a line of string
			void _m_draw_string(int top, const ::nana::color&, const nana::upoint& str_pos, const text_section&, bool if_mask) const;
			//_m_update_caret_line
			//@brief: redraw whole line specified by caret pos. 
			//@return: true if caret overs the border
			bool _m_update_caret_line(std::size_t secondary_before);

			void _m_offset_y(int y);

			unsigned _m_char_by_pixels(const unicode_bidi::entity&, unsigned pos) const;

			unsigned _m_pixels_by_char(const ::std::wstring&, ::std::size_t pos) const;
			void _m_handle_move_key(const arg_keyboard& arg);

			void _m_draw_border();
		private:
			struct implementation;
			implementation * const impl_;

			nana::window window_;
			graph_reference graph_;
			const text_editor_scheme*	scheme_;
			event_interface *			event_handler_{ nullptr };

			wchar_t mask_char_{0};

			struct attributes
			{
				::std::string tip_string;

				::nana::align alignment{ ::nana::align::left };

				bool line_wrapped{false};
				bool multi_lines{true};
				bool editable{true};
				bool enable_caret{ true }; ///< Indicates whether to show or hide caret when text_editor is not editable
				bool enable_background{true};
			}attributes_;

			struct text_area_type
			{
				nana::rectangle	area;

				bool		captured;
				unsigned	tab_space;
				unsigned	scroll_pixels;
				unsigned	vscroll;
				unsigned	hscroll;
			}text_area_;

			struct selection
			{
				enum class mode{ no_selected, mouse_selected, method_selected, move_selected };

				text_focus_behavior behavior;
				bool move_to_end;
				mode mode_selection;
				bool ignore_press;
				nana::upoint a, b;
			}select_;

			struct coordinate
			{
				nana::point		offset;	//x stands for pixels, y for lines
				nana::upoint	caret;	//position of caret by text, it specifies the position of a new character
				nana::upoint	shift_begin_caret;
				unsigned		xpos{0};	//This data is used for move up/down
			}points_;
		};
	}//end namespace skeletons
}//end namespace widgets
}//end namespace nana

#include <nana/pop_ignore_diagnostic>

#endif

