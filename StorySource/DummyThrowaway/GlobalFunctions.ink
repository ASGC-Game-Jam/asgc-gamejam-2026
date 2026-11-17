LIST moods = Elated, Deflated, Distracted, Pleasant, Frustrated, Shocked
LIST chars = rh, gb, ee, sb
VAR t_rh = 0
VAR t_gb = 0
VAR t_ee = 0
VAR t_sb = 0

// --- Set speaker and dialogue ---
=== function Set_Speaker(char_name, mood_int) ===
~ temp set_mood = moods(mood_int)
~ temp mood_text = "{set_mood}"
~ temp spk = ""
{ char_name:
    - rh: ~ spk = "RANHUMA"
    - gb: ~ spk = "GUMBOH"
    - ee: ~ spk = "EELIS"
    - sb: ~ spk = "SABELLE"
}
~ return "<center>" + spk + " (" + mood_text + ") </center>"

=== function Set_Dialogue(my_dialogue) ===
~  return "<center><i>"+my_dialogue+"</i></center>"

// --- Print character / speaker name ---
=== function Print_Char_Name(char_name) ===
~ temp char = ""
{ char_name:
    - rh: ~ char = "Ranhuma"
    - gb: ~ char = "Gumboh"
    - ee: ~ char = "Eelis"
    - sb: ~ char = "Sabelle"
}
~ return char

=== function Print_Speaker_Name(char_name) ===
~ temp spk = ""
{ char_name:
    - rh: ~ spk = "RANHUMA"
    - gb: ~ spk = "GUMBOH"
    - ee: ~ spk = "EELIS"
    - sb: ~ spk = "SABELLE"
}
~ return spk

// --- Set trust levels ---

=== function Set_Trust(char, amt) ===
{ char:
    - rh: ~ t_rh = amt
    - gb: ~ t_gb = amt
    - ee: ~ t_ee = amt
    - sb: ~ t_sb = amt
}
~ return

=== function Inc_Trust(char, amt) ===
{ char:
    - rh: ~ t_rh += amt
    - gb: ~ t_gb += amt
    - ee: ~ t_ee += amt
    - sb: ~ t_sb += amt
}
~ return "<center>GAINED TRUST WITH " + Print_Speaker_Name(char) + ".</center>"

=== function Dec_Trust(char, amt) ===
{ char:
    - rh: ~ t_rh -= amt
    - gb: ~ t_gb -= amt
    - ee: ~ t_ee -= amt
    - sb: ~ t_sb -= amt
}
~ return "<center>LOST TRUST WITH " + Print_Speaker_Name(char) + ".</center>"

=== function Print_Trust(char) ===
~ temp t_amt = ""
{ char:
    - rh: ~ t_amt = t_rh
    - gb: ~ t_amt = t_gb
    - ee: ~ t_amt = t_ee
    - sb: ~ t_amt = t_sb
}
~ return "<center>" + Print_Speaker_Name(char) + " TRUST LVL: " + t_amt + "/100</center>"