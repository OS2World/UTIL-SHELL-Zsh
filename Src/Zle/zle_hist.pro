void remember_edits _((void));
void forget_edits _((void));
void uphistory _((void));
void uplineorhistory _((void));
void viuplineorhistory _((void));
void uplineorsearch _((void));
void downlineorhistory _((void));
void vidownlineorhistory _((void));
void downlineorsearch _((void));
void acceptlineanddownhistory _((void));
void downhistory _((void));
void historysearchbackward _((void));
void historysearchforward _((void));
void beginningofbufferorhistory _((void));
void beginningofhistory _((void));
void endofbufferorhistory _((void));
void endofhistory _((void));
void insertlastword _((void));
char * qgetevent _((int ev));
char * zle_get_event _((int ev));
void pushline _((void));
void pushlineoredit _((void));
void pushinput _((void));
void getline _((void));
void historyincrementalsearchbackward _((void));
void historyincrementalsearchforward _((void));
void doisearch _((int dir));
void acceptandinfernexthistory _((void));
void infernexthistory _((void));
void vifetchhistory _((void));
int getvisrchstr _((void));
void vihistorysearchforward _((void));
void vihistorysearchbackward _((void));
void virepeatsearch _((void));
void virevrepeatsearch _((void));
void historybeginningsearchbackward _((void));
void historybeginningsearchforward _((void));
