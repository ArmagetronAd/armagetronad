#include <libxml/xmlreader.h>
#include <iostream>

#include "eChat.h"
#include "tString.h"

namespace test
{
    
// struct Session
// {
//     Session(const tString & name) :player_(name) { }
//     void AddSaid(const tString & say, const nTimeRolling & time)
//     {
//         player_.lastSaid_.Add
//     }
//     
//     Player player_;
// };


}

int main( int argc, const char *argv[] )
{
    if ( argc != 2 )
    {
        std::cerr << "Usage: chat_prefix_test <chat.xml>\n";
        return 1;
    }
    
    const char *filename = argv[1];
    
    int status;
    xmlTextReaderPtr reader = xmlNewTextReaderFilename(filename);
    do
    {
        status = xmlTextReaderRead(reader);
        
    } while ( status == 1 );
    xmlFreeTextReader( reader );
    
    return 0;
}
