class eCoord;
class eGrid;
class zZone;

//! creates a win or death zone (according to configuration) at the specified position
extern zZone * sz_CreateTimedZone( eGrid *, const eCoord & pos );
