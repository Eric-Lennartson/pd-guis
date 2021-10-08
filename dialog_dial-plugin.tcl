package require wheredoesthisgo

namespace eval ::dial_dialog:: {
}

proc ::dial_dialog::cancel {mytoplevel} {
    pdsend "$mytoplevel cancel"
}

proc ::dial_dialog::escape {sym} {
    # if length is 0 return -
    if {[string length $sym] == 0} {
        set ret "__EMPTY__"
    } else {
        # turn all $ to #
        set ret [string map {"$" "#"} $sym]
    }
    # clean string of unfriendly characters
    return [unspace_text $ret]
}

proc ::dial_dialog::unescape {sym} {
    return [string map {"#" "$"} $sym]
}

proc ::dial_dialog::set_ticks_state {} {
    if {$::discrete} {
        $::w.appearance.ticks_entry state !disabled
    } else {
        $::w.appearance.ticks_entry state disabled
    }
}

proc ::dial_dialog::set_inf_state {} {
    if {$::move_on_click} {
        set ::infinite 0
        $::w.behavior.tgls_frame.infinite_tgl state disabled
    } else {
        $::w.behavior.tgls_frame.infinite_tgl state !disabled
    }
}

proc ::dial_dialog::set_start_end_move_state {} {
    if {$::infinite} {
        $::w.appearance.start_angle_entry state disabled
        $::w.appearance.end_angle_entry state disabled
        set ::move_on_click 0
        $::w.behavior.tgls_frame.move_on_click_tgl state disabled
    } else {
        $::w.appearance.start_angle_entry state !disabled
        $::w.appearance.end_angle_entry state !disabled
        $::w.behavior.tgls_frame.move_on_click_tgl state !disabled
    }
}

proc ::dial_dialog::apply {mytoplevel} {
    pdsend "$mytoplevel _param \
            $::infinite \
            $::discrete \
            $::move_on_click \
            $::drag_mode \
            [::dial_dialog::escape [string range $::hotkey 0 0]] \
            $::show_ttip $::ttip_position \
            $::size \
            $::start_angle $::end_angle \
            $::start_position \
            $::n_ticks \
            $::min $::max \
            [::dial_dialog::escape $::send_name] \
            [::dial_dialog::escape $::rcv_name] \
            $::fine_scale $::macro_scale $::arrow_scale \
            [::dial_dialog::escape $::label_text] \
            $::label_position"
            # $::bg_color $::track_color $::ticks_color \
            # $::thumb_color $::thumb_highlight_color \
            # $::active_color"
}

proc ::dial_dialog::ok {mytoplevel} {
    ::dial_dialog::apply $mytoplevel
    ::dial_dialog::cancel $mytoplevel
}

# proc ::dial_dialog::update_color {mytoplevel} {
#     #for OSX live updates
#     if {$::windowingsystem eq "aqua"} {
#         ::dial_dialog::apply_and_rebind_return $mytoplevel
#     }
# }

# proc ::dial_dialog::set_color {mytoplevel which} {
#     switch $which {
#         "bg" {
#             set new_color [tk_chooseColor -title "Background Color" -initialcolor $::bg_color]
#             set which_color ::bg_color
#         }
#         "track" {
#             set new_color [tk_chooseColor -title "Track Color" -initialcolor $::track_color]
#             set which_color ::track_color
#         }
#         "ticks" {
#             set new_color [tk_chooseColor -title "Ticks Color" -initialcolor $::ticks_color]
#             set which_color ::ticks_color
#         }
#         "thumb" {
#             set new_color [tk_chooseColor -title "Thumb Color" -initialcolor $::thumb_color]
#             set which_color ::thumb_color
#         }
#         "thumb_highlight" {
#             set new_color [tk_chooseColor -title "Thumb Highlight Color" -initialcolor $::thumb_highlight_color]
#             set which_color ::thumb_highlight_color
#         }
#         "active" {
#             set new_color [tk_chooseColor -title "Active Color" -initialcolor $::active_color]
#             set which_color ::active_color
#         }
#     }

#     if {$new_color ne ""} {
#         switch $which {
#             "bg" {
#                 set ::bg_color $new_color
#                 ttk::style configure bg.TFrame -background $::bg_color
#             }
#             "track" {
#                 set ::track_color $new_color
#                 ttk::style configure track.TFrame -background $::track_color
#             }
#             "ticks" {
#                 set ::ticks_color $new_color
#                 ttk::style configure ticks.TFrame -background $::ticks_color
#             }
#             "thumb" {
#                 set ::thumb_color $new_color
#                 ttk::style configure thumb.TFrame -background $::thumb_color
#             }
#             "thumb_highlight" {
#                 set ::thumb_highlight_color $new_color
#                 ttk::style configure thumb_highlight.TFrame -background $::thumb_highlight_color
#             }
#             "active" {
#                 set ::active_color $new_color
#                 ttk::style configure active.TFrame -background $::active_color
#             }
#         }
#         update idletasks
#     }
#     ::dial_dialog::update_color $mytoplevel
# }

proc ::dial_dialog::dial_dialog { mytoplevel infinite discrete move_on_click drag_mode \
                  hotkey show_ttip ttip_position size start_angle end_angle \
                  start_position n_ticks min max send_name rcv_name fine_scale \
                  macro_scale arrow_scale label_text label_position } {

    if {[winfo exists $mytoplevel]} {
        wm deiconify $mytoplevel
        raise $mytoplevel
        focus $mytoplevel
    } else {
        set ::send_name ""
        set ::rcv_name ""
        set ::label_text ""
        if {$send_name ne "__EMPTY__"} {
            set ::send_name [unspace_text [::dial_dialog::unescape $send_name]]
        }
        if {$rcv_name ne "__EMPTY__"} {
            set ::rcv_name [unspace_text [::dial_dialog::unescape $rcv_name]]
        }
        if {$label_text ne "__EMPTY__"} {
            set ::label_text [unspace_text [::dial_dialog::unescape $label_text]]
        }

        # mytoplevel goes out of scope for the color frame bindings
        # make a global one to solve this.
        # set ::mtl $mytoplevel

        set ::infinite              $infinite
        set ::discrete              $discrete
        set ::move_on_click         $move_on_click
        set ::drag_mode             $drag_mode
        set ::hotkey                $hotkey
        set ::show_ttip             $show_ttip
        set ::ttip_position         $ttip_position
        set ::size                  $size
        set ::start_angle           $start_angle
        set ::end_angle             $end_angle
        set ::start_position        $start_position
        set ::n_ticks               $n_ticks
        set ::min                   $min
        set ::max                   $max
        set ::fine_scale            $fine_scale
        set ::macro_scale           $macro_scale
        set ::arrow_scale           $arrow_scale
        set ::label_position        $label_position

        # set ::track_color           $track_color
        # set ::ticks_color           $ticks_color
        # set ::thumb_color           $thumb_color
        # set ::thumb_highlight_color $thumb_highlight_color
        # set ::active_color          $active_color

        ::dial_dialog::create_dialog $mytoplevel
    }
}

proc ::dial_dialog::create_dialog {mytoplevel} {
    toplevel $mytoplevel
    wm title $mytoplevel "Dial Properties"
    wm group $mytoplevel .
    wm resizable $mytoplevel 0 0
    wm transient $mytoplevel $::focused_window
    $mytoplevel configure -menu $::dialog_menubar
    ::pd_bindings::dialog_bindings $mytoplevel "gatom"

    # ttk::style configure bg.TFrame              -background $::bg_color              -relief groove
    # ttk::style configure track.TFrame           -background $::track_color           -relief groove
    # ttk::style configure ticks.TFrame           -background $::ticks_color           -relief groove
    # ttk::style configure thumb.TFrame           -background $::thumb_color           -relief groove
    # ttk::style configure thumb_highlight.TFrame -background $::thumb_highlight_color -relief groove
    # ttk::style configure active.TFrame          -background $::active_color          -relief groove


    ttk::frame $mytoplevel.frame -padding 5
    set ::w $mytoplevel.frame

    # WIDGETS ##########################################################

    # Behavior Widgets #################################################
    # infinite, discrete, move on click, drag mode, hot key
    ttk::labelframe $::w.behavior -text " Behavior " -padding "3 3 2 1"
        ttk::frame $::w.behavior.tgls_frame
            ttk::label $::w.behavior.tgls_frame.infinite_label -text "Infinite:"
            ttk::checkbutton $::w.behavior.tgls_frame.infinite_tgl -variable ::infinite \
                -command "::dial_dialog::set_start_end_move_state"
            ttk::label $::w.behavior.tgls_frame.discrete_label -text "Discrete:"
            ttk::checkbutton $::w.behavior.tgls_frame.discrete_tgl -variable ::discrete \
                -command "::dial_dialog::set_ticks_state"
            ttk::label $::w.behavior.tgls_frame.move_on_click_label -text "Move On Click:"
            ttk::checkbutton $::w.behavior.tgls_frame.move_on_click_tgl -variable ::move_on_click \
                -command "::dial_dialog::set_inf_state"
        ttk::separator $::w.behavior.sep1
        ttk::frame $::w.behavior.drag_mode_frame -padding "0 2"
            ttk::label $::w.behavior.drag_mode_frame.label -text "Drag Mode:"
            ttk::frame $::w.behavior.drag_mode_frame.options_frame -padding "5 0 0 0"
                ttk::radiobutton $::w.behavior.drag_mode_frame.options_frame.horz -text "Horizontal"  \
                    -variable ::drag_mode -value 0
                ttk::radiobutton $::w.behavior.drag_mode_frame.options_frame.vert -text "Vertical" \
                    -variable ::drag_mode -value 1
                ttk::radiobutton $::w.behavior.drag_mode_frame.options_frame.horz_vert -text "Horizontal+Vertical" \
                    -variable ::drag_mode -value 2
                ttk::radiobutton $::w.behavior.drag_mode_frame.options_frame.rotary -text "Rotary" \
                    -variable ::drag_mode -value 3
        ttk::separator $::w.behavior.sep2
        ttk::frame $::w.behavior.hotkey_frame
            ttk::label $::w.behavior.hotkey_frame.label -text "Hotkey:"
            ttk::entry $::w.behavior.hotkey_frame.entry -width 1 -textvariable ::hotkey
    # Tooltip Widgets #################################################
    # position, show
    ttk::labelframe $::w.tooltip -text " Tooltip " -padding "3"
        ttk::label $::w.tooltip.show_label -text "Show:"
        ttk::checkbutton $::w.tooltip.show_tgl -variable ::show_ttip
        ttk::label $::w.tooltip.position_label -text "Position:"
        ttk::frame $::w.tooltip.positions_frame -padding "0 2"
            ttk::radiobutton $::w.tooltip.positions_frame.bot -text "Bottom" \
                -variable ::ttip_position -value 1
            ttk::radiobutton $::w.tooltip.positions_frame.left -text "Left" \
                -variable ::ttip_position -value 2
            ttk::radiobutton $::w.tooltip.positions_frame.top -text "Top" \
                -variable ::ttip_position -value 3
            ttk::radiobutton $::w.tooltip.positions_frame.right -text "Right" \
                -variable ::ttip_position -value 4
    # Appearance Widgets ##############################################
    # size, start angle, end angle, start position, ticks
    ttk::labelframe $::w.appearance -text " Appearance " -padding "3"
        ttk::label $::w.appearance.size_label -text "Size:"
        ttk::entry $::w.appearance.size_entry -width 5 -textvariable ::size
        ttk::label $::w.appearance.start_angle_label -text "Start Angle:"
        ttk::entry $::w.appearance.start_angle_entry -width 5 -textvariable ::start_angle
        ttk::label $::w.appearance.end_angle_label -text "End Angle:"
        ttk::entry $::w.appearance.end_angle_entry -width 5 -textvariable ::end_angle
        ttk::label $::w.appearance.start_position_label -text "Start Position:"
        ttk::frame $::w.appearance.positions_frame
            ttk::radiobutton $::w.appearance.positions_frame.bot -text "Bottom" \
                -variable ::start_position -value 1
            ttk::radiobutton $::w.appearance.positions_frame.left -text "Left" \
                -variable ::start_position -value 2
            ttk::radiobutton $::w.appearance.positions_frame.top -text "Top" \
                -variable ::start_position -value 3
            ttk::radiobutton $::w.appearance.positions_frame.right -text "Right" \
                -variable ::start_position -value 4
        ttk::label $::w.appearance.ticks_label -text "Ticks:"
        ttk::entry $::w.appearance.ticks_entry -width 5 -textvariable ::n_ticks
    # $::w.appearance.ticks_entry state disabled
    # Range Widgets ###################################################
    # minimum, maximum
    ttk::labelframe $::w.range -text " Range " -padding "3"
        ttk::label $::w.range.min_label -text "Minimum:"
        ttk::entry $::w.range.min_entry -width 5 -textvariable ::min
        ttk::label $::w.range.max_label -text "Maximum:"
        ttk::entry $::w.range.max_entry -width 5 -textvariable ::max
    # Messaging Widgets ###############################################
    # send receive
    ttk::labelframe $::w.messaging -text " Messaging " -padding "3"
        ttk::label $::w.messaging.send_label -text "Send:"
        ttk::entry $::w.messaging.send_entry -width 25 -textvariable ::send_name
        ttk::label $::w.messaging.receive_label -text "Receive:"
        ttk::entry $::w.messaging.receive_entry -width 25 -textvariable ::rcv_name
    # Scaling Widgets #################################################
    # fine tune, macro tune, arrow
    ttk::labelframe $::w.scaling -text " Scaling " -padding "3"
        ttk::label $::w.scaling.fine_tune_label -text "Fine:"
        ttk::entry $::w.scaling.fine_tune_entry -width 5 -textvariable ::fine_scale
        ttk::label $::w.scaling.macro_tune_label -text "Macro:"
        ttk::entry $::w.scaling.macro_tune_entry -width 5 -textvariable ::macro_scale
        ttk::label $::w.scaling.arrow_label -text "Arrow:"
        ttk::entry $::w.scaling.arrow_entry -width 5 -textvariable ::arrow_scale
    # Label Widgets ###################################################
    # label, position
    ttk::labelframe $::w.label -text " Label " -padding "3 3 3 1"
        ttk::label $::w.label.label_label -text "Label:"
        ttk::entry $::w.label.label_entry -width 25 -textvariable ::label_text
        ttk::label $::w.label.position_label -text "Position:"
        ttk::frame $::w.label.position_frame -padding "0 2"
            ttk::radiobutton $::w.label.position_frame.bot -text "Bottom" \
                -variable ::label_position -value 1
            ttk::radiobutton $::w.label.position_frame.left -text "Left" \
                -variable ::label_position -value 2
            ttk::radiobutton $::w.label.position_frame.top -text "Top" \
                -variable ::label_position -value 3
            ttk::radiobutton $::w.label.position_frame.right -text "Right" \
                -variable ::label_position -value 4
    # Colors Widgets ##################################################
    # background, track, ticks, thumb, thumb highlight, active
    ttk::labelframe $::w.colors -text " Colors " -padding "5 2 0 2"
        ttk::label $::w.colors.bg_label -text "Background:"
        ttk::frame $::w.colors.bg_preview -width 46 -height 16 -style bg.TFrame
        ttk::label $::w.colors.track_label -text "Track:"
        ttk::frame $::w.colors.track_preview -width 46 -height 16 -style track.TFrame
        ttk::label $::w.colors.ticks_label -text "Ticks:"
        ttk::frame $::w.colors.ticks_preview -width 46 -height 16 -style ticks.TFrame
        ttk::label $::w.colors.thumb_label -text "Thumb:"
        ttk::frame $::w.colors.thumb_preview -width 46 -height 16 -style thumb.TFrame
        ttk::label $::w.colors.thumb_highlight_label -text "Thumb Highlight:"
        ttk::frame $::w.colors.thumb_highlight_preview -width 46 -height 16 -style thumb_highlight.TFrame
        ttk::label $::w.colors.active_label -text "Active:"
        ttk::frame $::w.colors.active_preview -width 46 -height 16 -style active.TFrame
    # Button Widgets #################################################
    ttk::frame $::w.buttons
        ttk::button $::w.buttons.cancel -text "Cancel" \
            -command "::dial_dialog::cancel $mytoplevel"
        ttk::button $::w.buttons.apply -text "Apply" \
            -command "::dial_dialog::apply $mytoplevel"
        ttk::button $::w.buttons.ok -text "OK" \
            -command "::dial_dialog::ok $mytoplevel" -default active

    ###################################################################
    # LAYOUT ##########################################################
    ###################################################################
    grid $::w -column 0 -row 0 -sticky nwes

    # Appearance Layout ##############################################
    # size, start angle, end angle, start position, ticks
    grid $::w.appearance -column 0 -row 0 -sticky nwes
        grid $::w.appearance.size_label           -column 0 -row 0 -sticky w
        grid $::w.appearance.size_entry           -column 1 -row 0 -sticky w
        grid $::w.appearance.start_angle_label    -column 0 -row 1 -sticky w
        grid $::w.appearance.start_angle_entry    -column 1 -row 1 -sticky w
        grid $::w.appearance.end_angle_label      -column 2 -row 1 -sticky w
        grid $::w.appearance.end_angle_entry      -column 3 -row 1 -sticky w
        grid $::w.appearance.ticks_label          -column 0 -row 2 -sticky w
        grid $::w.appearance.ticks_entry          -column 1 -row 2 -sticky w
        grid $::w.appearance.start_position_label -column 0 -row 3 -sticky w
        grid $::w.appearance.positions_frame      -column 1 -row 3 -columnspan 3
            grid $::w.appearance.positions_frame.right -column 0 -row 0
            grid $::w.appearance.positions_frame.left  -column 1 -row 0
            grid $::w.appearance.positions_frame.top   -column 2 -row 0
            grid $::w.appearance.positions_frame.bot   -column 3 -row 0

    # Behavior Layout #################################################
    # infinite, discrete, move on click, drag mode, hot key
    grid $::w.behavior -column 0 -row 1 -sticky nwes
        grid $::w.behavior.tgls_frame -column 0 -row 0 -sticky nwes
            grid $::w.behavior.tgls_frame.infinite_label      -column 0 -row 0 -sticky w
            grid $::w.behavior.tgls_frame.infinite_tgl        -column 1 -row 0 -sticky w -padx 3
            grid $::w.behavior.tgls_frame.discrete_label      -column 2 -row 0 -sticky w
            grid $::w.behavior.tgls_frame.discrete_tgl        -column 3 -row 0 -sticky w -padx 3
            grid $::w.behavior.tgls_frame.move_on_click_label -column 4 -row 0 -sticky w
            grid $::w.behavior.tgls_frame.move_on_click_tgl   -column 5 -row 0 -sticky w -padx 3
        grid $::w.behavior.sep1            -column 0 -row 3 -sticky we -pady 4
        grid $::w.behavior.drag_mode_frame -column 0 -row 4 -sticky w -sticky nwes
            grid $::w.behavior.drag_mode_frame.label -column 0 -row 0 -sticky w
            grid $::w.behavior.drag_mode_frame.options_frame -column 1 -row 0 -sticky nwes
                grid $::w.behavior.drag_mode_frame.options_frame.horz      -column 0 -row 0 -sticky w
                grid $::w.behavior.drag_mode_frame.options_frame.vert      -column 0 -row 1 -sticky w -pady 2
                grid $::w.behavior.drag_mode_frame.options_frame.horz_vert -column 1 -row 0 -sticky w
                grid $::w.behavior.drag_mode_frame.options_frame.rotary    -column 1 -row 1 -sticky w
        grid $::w.behavior.sep2         -column 0 -row 5 -sticky we -pady 2
        grid $::w.behavior.hotkey_frame -column 0 -row 6 -sticky nwes -pady 2
            grid $::w.behavior.hotkey_frame.label -column 0 -row 0 -sticky w
            grid $::w.behavior.hotkey_frame.entry -column 1 -row 0 -sticky w
    # Range Layout ##################################################
    # minimum, maximum
    grid $::w.range -column 0 -row 2 -sticky nwes
        grid $::w.range.min_label -column 0 -row 0 -sticky w
        grid $::w.range.min_entry -column 1 -row 0
        grid $::w.range.max_label -column 2 -row 0 -sticky w
        grid $::w.range.max_entry -column 3 -row 0
    # Tooltip Layout #################################################
    # position, show
    grid $::w.tooltip -column 0 -row 3 -sticky nwes
        grid $::w.tooltip.show_label      -column 0 -row 0 -sticky w
        grid $::w.tooltip.show_tgl        -column 1 -row 0 -sticky w
        grid $::w.tooltip.position_label  -column 0 -row 1 -sticky w
        grid $::w.tooltip.positions_frame -column 1 -row 1 -sticky w
            grid $::w.tooltip.positions_frame.right -column 0 -row 0
            grid $::w.tooltip.positions_frame.left  -column 1 -row 0
            grid $::w.tooltip.positions_frame.top   -column 2 -row 0
            grid $::w.tooltip.positions_frame.bot   -column 3 -row 0
    # Messaging Layout ##################################################
    # send, receive
    grid $::w.messaging -column 0 -row 4 -sticky nwes
        grid $::w.messaging.send_label    -column 0 -row 0 -sticky w
        grid $::w.messaging.send_entry    -column 1 -row 0 -sticky we
        grid $::w.messaging.receive_label -column 0 -row 1 -sticky w
        grid $::w.messaging.receive_entry -column 1 -row 1 -sticky we
    # Scaling Layout ##################################################
    # fine tune, macro tune, arrow
    grid $::w.scaling -column 0 -row 5 -sticky we
        grid $::w.scaling.fine_tune_label  -column 0 -row 0 -padx 1
        grid $::w.scaling.fine_tune_entry  -column 1 -row 0 -padx 2
        grid $::w.scaling.macro_tune_label -column 2 -row 0 -padx 1
        grid $::w.scaling.macro_tune_entry -column 3 -row 0 -padx 2
        grid $::w.scaling.arrow_label      -column 4 -row 0 -padx 1
        grid $::w.scaling.arrow_entry      -column 5 -row 0 -padx 2
    # Label Layout ##################################################
    # label, position
    grid $::w.label -column 0 -row 6 -sticky nwes
        grid $::w.label.label_label    -column 0 -row 0 -sticky w
        grid $::w.label.label_entry    -column 1 -row 0 -sticky w
        grid $::w.label.position_label -column 0 -row 1 -sticky w
        grid $::w.label.position_frame -column 1 -row 1 -pady 2
            grid $::w.label.position_frame.right -column 0 -row 1
            grid $::w.label.position_frame.left  -column 1 -row 1
            grid $::w.label.position_frame.top   -column 2 -row 1
            grid $::w.label.position_frame.bot   -column 3 -row 1
    # Colors Layout ##################################################
    # background, track, ticks, thumb, thumb highlight, active
    # grid $::w.colors -column 0 -row 7 -sticky nwes
    #     grid $::w.colors.bg_label                -column 0 -row 0 -sticky w
    #     grid $::w.colors.bg_preview              -column 1 -row 0
    #     grid $::w.colors.thumb_label             -column 2 -row 0 -sticky w
    #     grid $::w.colors.thumb_preview           -column 3 -row 0
    #     grid $::w.colors.track_label             -column 0 -row 1 -sticky w
    #     grid $::w.colors.track_preview           -column 1 -row 1 -pady 2
    #     grid $::w.colors.thumb_highlight_label   -column 2 -row 1 -sticky w
    #     grid $::w.colors.thumb_highlight_preview -column 3 -row 1 -pady 2
    #     grid $::w.colors.ticks_label             -column 0 -row 2 -sticky w
    #     grid $::w.colors.ticks_preview           -column 1 -row 2 -pady 1
    #     grid $::w.colors.active_label            -column 2 -row 2 -sticky w
    #     grid $::w.colors.active_preview          -column 3 -row 2 -pady 1
    # Buttons Layout ##################################################
    grid $::w.buttons        -column 0 -row 8 -sticky ns ;# no west east so that we stay centered vertically
        grid $::w.buttons.ok     -column 0 -row 0
        grid $::w.buttons.apply  -column 1 -row 0
        grid $::w.buttons.cancel -column 2 -row 0

    # use scaling b/c it is the widest (WARNING this could change later)
    update idletasks
    set width [winfo reqwidth $::w.scaling]\n

    # overdesigned and hacky, but I think it's cool. I'll keep it around
    ttk::labelframe $::w.appearance_collapsed -text " Appearance " -height 17 -width $width
    ttk::labelframe $::w.behavior_collapsed -text " Behavior " -height 17 -width $width
    ttk::labelframe $::w.range_collapsed -text " Range " -height 17 -width $width
    ttk::labelframe $::w.tooltip_collapsed -text " Tooltip " -height 17 -width $width
    ttk::labelframe $::w.messaging_collapsed -text " Messaging " -height 17 -width $width
    ttk::labelframe $::w.scaling_collapsed -text " Scaling " -height 17 -width $width
    ttk::labelframe $::w.label_collapsed -text " Label " -height 17 -width $width
    # ttk::labelframe $::w.colors_collapsed -text " Colors " -height 17 -width $width

    bind $::w.appearance <ButtonPress> {
        grid remove $::w.appearance
        grid $::w.appearance_collapsed -column 0 -row 0 -sticky nwes
    }

    bind $::w.appearance_collapsed <ButtonPress> {
        grid remove $::w.appearance_collapsed
        grid $::w.appearance
        update idletasks
    }

    bind $::w.behavior <ButtonPress> {
        grid remove $::w.behavior
        grid $::w.behavior_collapsed -column 0 -row 1 -sticky nwes
    }

    bind $::w.behavior_collapsed <ButtonPress> {
        grid remove $::w.behavior_collapsed
        grid $::w.behavior
        update idletasks
    }

    bind $::w.range <ButtonPress> {
        grid remove $::w.range
        grid $::w.range_collapsed -column 0 -row 2 -sticky nwes
    }

    bind $::w.range_collapsed <ButtonPress> {
        grid remove $::w.range_collapsed
        grid $::w.range
        update idletasks
    }

    bind $::w.tooltip <ButtonPress> {
        grid remove $::w.tooltip
        grid $::w.tooltip_collapsed -column 0 -row 3 -sticky nwes
    }

    bind $::w.tooltip_collapsed <ButtonPress> {
        grid remove $::w.tooltip_collapsed
        grid $::w.tooltip
        update idletasks
    }

    bind $::w.messaging <ButtonPress> {
        grid remove $::w.messaging
        grid $::w.messaging_collapsed -column 0 -row 4 -sticky nwes
    }

    bind $::w.messaging_collapsed <ButtonPress> {
        grid remove $::w.messaging_collapsed
        grid $::w.messaging
        update idletasks
    }

    bind $::w.scaling <ButtonPress> {
        grid remove $::w.scaling
        grid $::w.scaling_collapsed -column 0 -row 5 -sticky nwes
    }

    bind $::w.scaling_collapsed <ButtonPress> {
        grid remove $::w.scaling_collapsed
        grid $::w.scaling
        update idletasks
    }

    bind $::w.label <ButtonPress> {
        grid remove $::w.label
        grid $::w.label_collapsed -column 0 -row 6 -sticky nwes
    }

    bind $::w.label_collapsed <ButtonPress> {
        grid remove $::w.label_collapsed
        grid $::w.label
        update idletasks
    }

    # bind $::w.colors <ButtonPress> {
    #     grid remove $::w.colors
    #     grid $::w.colors_collapsed -column 0 -row 7 -sticky nwes
    # }

    # bind $::w.colors_collapsed <ButtonPress> {
    #     grid remove $::w.colors_collapsed
    #     grid $::w.colors
    #     update idletasks
    # }

    # click on frame to change colors
    # bind $::w.colors.bg_preview              <ButtonPress> {::dial_dialog::set_color $::mtl "bg"}
    # bind $::w.colors.track_preview           <ButtonPress> {::dial_dialog::set_color $::mtl "track"}
    # bind $::w.colors.ticks_preview           <ButtonPress> {::dial_dialog::set_color $::mtl "ticks"}
    # bind $::w.colors.thumb_preview           <ButtonPress> {::dial_dialog::set_color $::mtl "thumb"}
    # bind $::w.colors.thumb_highlight_preview <ButtonPress> {::dial_dialog::set_color $::mtl "thumb_highlight"}
    # bind $::w.colors.active_preview          <ButtonPress> {::dial_dialog::set_color $::mtl "active"}

    # Update the states
    ::dial_dialog::set_start_end_move_state
    ::dial_dialog::set_inf_state
    ::dial_dialog::set_ticks_state

    # live updates on macOS
    # TODO this doesn't include everything
    if {$::windowingsystem eq "aqua"} {
        # call apply on radiobutton changes
        $::w.behavior.drag_mode_frame.options_frame.horz config -command [ concat ::dial_dialog::apply $mytoplevel ]
        $::w.behavior.drag_mode_frame.options_frame.vert config -command [ concat ::dial_dialog::apply $mytoplevel ]
        $::w.behavior.drag_mode_frame.options_frame.horz_vert config -command [ concat ::dial_dialog::apply $mytoplevel ]
        $::w.behavior.drag_mode_frame.options_frame.rotary config -command [ concat ::dial_dialog::apply $mytoplevel ]
        $::w.tooltip.positions_frame.top config -command [ concat ::dial_dialog::apply $mytoplevel ]
        $::w.tooltip.positions_frame.right config -command [ concat ::dial_dialog::apply $mytoplevel ]
        $::w.tooltip.positions_frame.left config -command [ concat ::dial_dialog::apply $mytoplevel ]
        $::w.tooltip.positions_frame.bot config -command [ concat ::dial_dialog::apply $mytoplevel ]
        $::w.appearance.positions_frame.bot config -command [ concat ::dial_dialog::apply $mytoplevel ]
        $::w.appearance.positions_frame.top config -command [ concat ::dial_dialog::apply $mytoplevel ]
        $::w.appearance.positions_frame.right config -command [ concat ::dial_dialog::apply $mytoplevel ]
        $::w.appearance.positions_frame.left config -command [ concat ::dial_dialog::apply $mytoplevel ]
        $::w.label.position_frame.bot config -command [ concat ::dial_dialog::apply $mytoplevel ]
        $::w.label.position_frame.top config -command [ concat ::dial_dialog::apply $mytoplevel ]
        $::w.label.position_frame.right config -command [ concat ::dial_dialog::apply $mytoplevel ]
        $::w.label.position_frame.left config -command [ concat ::dial_dialog::apply $mytoplevel ]

        # allow radiobutton focus
        $::w.behavior.drag_mode_frame.options_frame.horz config -takefocus 1
        $::w.behavior.drag_mode_frame.options_frame.vert config -takefocus 1
        $::w.behavior.drag_mode_frame.options_frame.horz_vert config -takefocus 1
        $::w.behavior.drag_mode_frame.options_frame.rotary config -takefocus 1
        $::w.tooltip.positions_frame.top config -takefocus 1
        $::w.tooltip.positions_frame.right config -takefocus 1
        $::w.tooltip.positions_frame.left config -takefocus 1
        $::w.tooltip.positions_frame.bot config -takefocus 1
        $::w.appearance.positions_frame.bot config -takefocus 1
        $::w.appearance.positions_frame.top config -takefocus 1
        $::w.appearance.positions_frame.right config -takefocus 1
        $::w.appearance.positions_frame.left config -takefocus 1
        $::w.label.position_frame.bot config -takefocus 1
        $::w.label.position_frame.top config -takefocus 1
        $::w.label.position_frame.right config -takefocus 1
        $::w.label.position_frame.left config -takefocus 1

        # call apply on Return in entry boxes that are in focus & rebind Return to ok button
        bind $::w.behavior.hotkey_frame.entry <KeyPress-Return> "::dial_dialog::apply_and_rebind_return $mytoplevel"
        bind $::w.appearance.size_entry <KeyPress-Return> "::dial_dialog::apply_and_rebind_return $mytoplevel"
        bind $::w.appearance.start_angle_entry <KeyPress-Return> "::dial_dialog::apply_and_rebind_return $mytoplevel"
        bind $::w.appearance.end_angle_entry <KeyPress-Return> "::dial_dialog::apply_and_rebind_return $mytoplevel"
        bind $::w.appearance.ticks_entry <KeyPress-Return> "::dial_dialog::apply_and_rebind_return $mytoplevel"
        bind $::w.range.min_entry <KeyPress-Return> "::dial_dialog::apply_and_rebind_return $mytoplevel"
        bind $::w.range.max_entry <KeyPress-Return> "::dial_dialog::apply_and_rebind_return $mytoplevel"
        bind $::w.messaging.send_entry <KeyPress-Return> "::dial_dialog::apply_and_rebind_return $mytoplevel"
        bind $::w.messaging.receive_entry <KeyPress-Return> "::dial_dialog::apply_and_rebind_return $mytoplevel"
        bind $::w.scaling.fine_tune_entry <KeyPress-Return> "::dial_dialog::apply_and_rebind_return $mytoplevel"
        bind $::w.scaling.macro_tune_entry <KeyPress-Return> "::dial_dialog::apply_and_rebind_return $mytoplevel"
        bind $::w.scaling.arrow_entry <KeyPress-Return> "::dial_dialog::apply_and_rebind_return $mytoplevel"
        bind $::w.label.label_entry <KeyPress-Return> "::dial_dialog::apply_and_rebind_return $mytoplevel"

        # unbind Return from ok button when an entry takes focus
        $::w.behavior.hotkey_frame.entry config -validate focusin -validatecommand "::dial_dialog::unbind_return $mytoplevel"
        $::w.appearance.size_entry config -validate focusin -validatecommand "::dial_dialog::unbind_return $mytoplevel"
        $::w.appearance.start_angle_entry config -validate focusin -validatecommand "::dial_dialog::unbind_return $mytoplevel"
        $::w.appearance.end_angle_entry config -validate focusin -validatecommand "::dial_dialog::unbind_return $mytoplevel"
        $::w.appearance.ticks_entry config -validate focusin -validatecommand "::dial_dialog::unbind_return $mytoplevel"
        $::w.range.min_entry config -validate focusin -validatecommand "::dial_dialog::unbind_return $mytoplevel"
        $::w.range.max_entry config -validate focusin -validatecommand "::dial_dialog::unbind_return $mytoplevel"
        $::w.messaging.send_entry config -validate focusin -validatecommand "::dial_dialog::unbind_return $mytoplevel"
        $::w.messaging.receive_entry config -validate focusin -validatecommand "::dial_dialog::unbind_return $mytoplevel"
        $::w.scaling.fine_tune_entry config -validate focusin -validatecommand "::dial_dialog::unbind_return $mytoplevel"
        $::w.scaling.macro_tune_entry config -validate focusin -validatecommand "::dial_dialog::unbind_return $mytoplevel"
        $::w.scaling.arrow_entry config -validate focusin -validatecommand "::dial_dialog::unbind_return $mytoplevel"
        $::w.label.label_entry config -validate focusin -validatecommand "::dial_dialog::unbind_return $mytoplevel"

        # remove cancel button from focus list since it's not activated on Return
        $::w.buttons.cancel config -takefocus 0

        # show active focus on the ok button as it *is* activated on Return
        $::w.buttons.ok config -default normal
        bind $::w.buttons.ok <FocusIn> "$::w.buttons.ok config -default active"
        bind $::w.buttons.ok <FocusOut> "$::w.buttons.ok config -default normal"
    }
}

# for live widget updates on OSX
proc ::dial_dialog::apply_and_rebind_return {mytoplevel} {
    ::dial_dialog::apply $mytoplevel
    bind $mytoplevel <KeyPress-Return> "::dial_dialog::ok $mytoplevel"
    focus $::w.buttons.ok
    return 0
}

# for live widget updates on OSX
proc ::dial_dialog::unbind_return {mytoplevel} {
    bind $mytoplevel <KeyPress-Return> break
    return 1
}
