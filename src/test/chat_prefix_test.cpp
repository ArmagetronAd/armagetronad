#include <libxml/xmlreader.h>
#include <iostream>

#include "eChat.h"
#include "tString.h"
#include "ePlayer.h"

struct Stats
{
    Stats() : sessions( 0 ), chats( 0 ), chatsThrough( 0 ), foundPrefixes( 0 ) { }
    int sessions;
    int chats;
    int chatsThrough;
    int foundPrefixes;
    
    static Stats stats;
};

Stats Stats::stats = Stats();


tString ConvertXMLString( const xmlChar *x )
{
    return tString( (const char *)x );
}

struct Session
{
    Session( int lineNumber ) :lineNumber_( lineNumber ), player_(), chatlog_() { }
    
    void AddSaid( const tString & say , nTimeRolling time )
    {
        eChatSaidEntry entry( say, time, eChatMessageType_Public );
        chatlog_.push_back( entry );
    }
    
    int lineNumber_;
    tString player_;
    std::vector< eChatSaidEntry > chatlog_;
};

void TestSession( const Session & session )
{
    Stats::stats.sessions += 1;
    
    ePlayerNetID player;
    player.SetName( session.player_ );
    
    for ( size_t i = 0; i < session.chatlog_.size(); i++)
    {
        Stats::stats.chats += 1;
        
        const eChatSaidEntry & entry = session.chatlog_[i];
        
        eChatPrefixSpamTester tester( &player, entry );
        
        tString out;
        nTimeRolling timeOut;
        eChatPrefixSpamType typeOut;
        
        if ( tester.Check( out, timeOut, typeOut ) )
        {
            if ( typeOut != eChatPrefixSpamType_Known )
            {
                std::cout << "Found from session starting at line number " << session.lineNumber_ << "\n\n";
                Stats::stats.foundPrefixes += 1;
            }
        }
        else
        {
            Stats::stats.chatsThrough += 1;
            player.lastSaid_.AddSaid( entry );
        }
    }
}

void ProcessNode( xmlTextReaderPtr reader )
{
    static Session currentSession( 0 );
    
    int type = xmlTextReaderNodeType( reader );
    
    if ( type == XML_READER_TYPE_END_ELEMENT && xmlStrEqual( xmlTextReaderConstName( reader ), BAD_CAST "Session" ) )
    {
        TestSession( currentSession );
    }
    
    if ( type != XML_READER_TYPE_ELEMENT )
        return;
    
    if ( xmlStrEqual( xmlTextReaderConstName( reader ), BAD_CAST "Session" ) )
    {
        currentSession = Session( xmlTextReaderGetParserLineNumber( reader ) );
    }
    else if ( xmlStrEqual( xmlTextReaderConstName( reader ), BAD_CAST "Player" ) )
    {
        xmlChar *xmlPlayer = xmlTextReaderReadString( reader );
        currentSession.player_ = ConvertXMLString( xmlPlayer );
        xmlFree( xmlPlayer );
    }
    else if ( xmlStrEqual( xmlTextReaderConstName( reader ), BAD_CAST "Said" ) )
    {
        xmlChar *xmlTime = xmlTextReaderGetAttribute( reader, BAD_CAST "time" );
        xmlChar *xmlSaid = xmlTextReaderReadString( reader );
        
        long time = atol( (const char *)xmlTime );
        tString say = ConvertXMLString( xmlSaid );
        
        currentSession.AddSaid( say, time );
        
        xmlFree( xmlTime );
        xmlFree( xmlSaid );
    }
}

bool Parse( const char *filename )
{
    int status;
    xmlTextReaderPtr reader = xmlNewTextReaderFilename( filename );
    if ( reader != NULL )
    {
        status = xmlTextReaderRead( reader );
        while ( status == 1 )
        {
            ProcessNode( reader );
            status = xmlTextReaderRead(reader);
        }
        xmlFreeTextReader( reader );
        
        if ( status != 0 )
        {
            std::cerr << "Parsing error\n";
            return false;
        }
    }
    else
    {
        std::cerr << "Unable to open '" << filename << "'\n";
        return false;
    }
    
    return true;
}

int main( int argc, const char *argv[] )
{
    if ( argc != 2 )
    {
        std::cerr << "Usage: chat_prefix_test <chat.xml>\n";
        return 1;
    }
    
    int success = Parse( argv[1] ) ? 0 : 1;
    
    std::cout << "chat_prefix_test done!\n";
    std::cout << "Statistics:\n\tNumber of sessions tested: " << Stats::stats.sessions
              << "\n\tNumber of chats: " << Stats::stats.chats
              << "\n\tNumber of chats let through: " << Stats::stats.chatsThrough
              << "\n\tNumber of chats blocked: " << Stats::stats.chats - Stats::stats.chatsThrough
              << "\n\tNumber of chat prefixes found: " << Stats::stats.foundPrefixes << "\n";
    return success;
}
