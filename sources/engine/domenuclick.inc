/* body of DoMenuClick: the head is in mainscreen.c and, under MAC1010, in maininfo.c */
{
    short menu;
    short item;

    menu = HiWord(menuChoice);
    item = LoWord(menuChoice);
    if (menu == kAppleMenu) {
        if (item == kAboutItem)
            DoAboutBox();
    } else if (menu == kFileMenu) {
        if (item == kQuitItem)
            gQuit = 1;
    } else if (menu == kEditMenu) {
        if (item == kPrefsItem && gameLoaded)
            DoPrefsDialog();
    }
    HiliteMenu(0);
}
