#include "tBannedWords.h"
#include "tConfiguration.h"
#include "tDirectories.h"
#include "tRecorder.h"
#include "nNetwork.h"

tBannedWords::tBannedWords()
{
    Clear();
}

tBannedWords *st_BannedWords = new tBannedWords();

static void st_BannedWordsStore(std::istream &s)
{
    tString wordSt;
    wordSt.ReadLine(s);

    if (wordSt.Filter() == "")
        return;

    st_BannedWords->Clear();

    tArray<tString> words = wordSt.Split(";");
    for(int i = 0; i < words.Len(); i++)
    {
        tString word = words[i];
        if (word.Filter() != "")
        {
            st_BannedWords->Add(word);
        }
    }
}
static tConfItemFunc st_BannedWordsStoreConf("BANNED_WORDS", &st_BannedWordsStore);

static void st_BannedWordsAdd(std::istream &s)
{
    tString word;
    s >> word;

    if (word.Filter() == "")
        return;

    bool found = false;
    if (st_BannedWords->Count() >= 0)
    {
        for(int i = 0; i < st_BannedWords->Count(); i++)
        {
            tString wordSearch = st_BannedWords->GetWord(i);
            if (wordSearch.Filter() == word.Filter())
            {
                found = true;
                break;
            }
        }
    }

    if (!found)
    {
        st_BannedWords->Add(word);
    }
}
static tConfItemFunc st_BannedWordsAddConf("BANNED_WORDS_ADD", &st_BannedWordsAdd);

static void st_BannedWordsRemove(std::istream &s)
{
    tString word;
    s >> word;

    if (word.Filter() == "")
        return;

    bool found = false;
    int wordId = -1;

    if (st_BannedWords->Count() >= 0)
    {
        for(int i = 0; i < st_BannedWords->Count(); i++)
        {
            tString wordSearch = st_BannedWords->GetWord(i);
            if (wordSearch.Filter() == word.Filter())
            {
                found = true;
                wordId = i;

                break;
            }
        }
    }

    if (found && wordId >= 0)
    {
        st_BannedWords->RemoveWord(wordId);
    }
}
static tConfItemFunc st_BannedWordsRemoveConf("BANNED_WORDS_REMOVE", &st_BannedWordsRemove);

static void st_BannedWordsList(std::istream &s)
{
    int max = 10;
    int showing = 0;

    tString params;
    int pos = 0;
    params.ReadLine(s);

    int showAmount = 0;
    tString amount = params.ExtractNonBlankSubString(pos);
    if (amount.Filter() != "") showAmount = atoi(amount);

    if (st_BannedWords->Count() > 0)
    {
        if (showAmount < st_BannedWords->Count())
        {
            if (st_BannedWords->Count() < max) max = st_BannedWords->Count();
            if ((st_BannedWords->Count() - showAmount) < max) max = st_BannedWords->Count() - showAmount;

            if (max > 0)
            {
                sn_ConsoleOut(tOutput("$banned_words_list"), 0);

                for(int i = 0; i < max; i++)
                {
                    int rotID = showAmount + i;
                    tString word = st_BannedWords->GetWord(i);
                    if (word.Filter() != "")
                    {
                        tColoredString send;
                        send << tColoredString::ColorString( 1,1,.5 );
                        //send << "( ";
                        send << "0xaacc09" << word;
                        send << tColoredString::ColorString( 1,1,.5 );
                        //send << " ),\n";
                        sn_ConsoleOut(send, 0);

                        showing++;
                    }
                }

                tOutput show;
                show.SetTemplateParameter(1, showing);
                show.SetTemplateParameter(2, st_BannedWords->Count());
                show << "$banned_words_list_show";
                sn_ConsoleOut(show, 0);
            }
        }
    }
}
static tConfItemFunc st_BannedWordsListConf("BANNED_WORDS_LIST", &st_BannedWordsList);
static tAccessLevelSetter st_BannedWordsListConfLevel( st_BannedWordsListConf, tAccessLevel_Moderator );

static int st_BannedWordsOptions = 0;
bool restrictBannedWordsOptionsValue(const int &newValue)
{
    if ((newValue < 0) || (newValue > 2))
        return false;

    return true;
}
static tSettingItem<int> st_BannedWordsOptionsConf("BANNED_WORDS_OPTIONS", st_BannedWordsOptions, &restrictBannedWordsOptionsValue);

bool tBannedWords::BadWordTrigger(ePlayerNetID *sender, tString &message)
{
    if ((st_BannedWords->Count() > 0) && (st_BannedWordsOptions > 0))
    {
        //  Loop through each bad word container and check in message if that bad word exists
        for (int wordID = 0; wordID < st_BannedWords->Count(); wordID++)
        {
            //  fetch the word currently in wordID
            tString word = st_BannedWords->BannedWordsList()[wordID];

            //  check if a banned word exists in the message
            if ((word.Filter() != "") && (message.Contains(word)))
            {
                //  option 1: alert the sender of the usage of banned word in their message
                if (st_BannedWordsOptions == 1)
                {
                    tOutput msg;
                    msg << "$banned_words_warning";
                    sn_ConsoleOut(msg, sender->Owner());

                    return true;
                }
                //  option 2: replace the banned word with the replacement word from the language setting
                else if (st_BannedWordsOptions == 2)
                {
                    tString replacementWord;
                    replacementWord = tOutput("$banned_words_replace");
                    message.Replace(word, replacementWord);
                }
            }
        }
    }

    return false;
}
