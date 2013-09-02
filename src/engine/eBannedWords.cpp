#include "eBannedWords.h"
#include "tConfiguration.h"
#include "tDirectories.h"
#include "tRecorder.h"
#include "nNetwork.h"

eBannedWords::eBannedWords()
{
    Clear();
}

eBannedWords *se_BannedWords = new eBannedWords();

static void se_BannedWordsStore(std::istream &s)
{
    tString wordSt;
    wordSt.ReadLine(s);

    if (wordSt.Filter() == "")
        return;

    se_BannedWords->Clear();

    tArray<tString> words = wordSt.Split(";");
    for(int i = 0; i < words.Len(); i++)
    {
        tString word = words[i];
        if (word.Filter() != "")
        {
            se_BannedWords->Add(word);
        }
    }
}
static tConfItemFunc se_BannedWordsStoreConf("BANNED_WORDS", &se_BannedWordsStore);

static void se_BannedWordsAdd(std::istream &s)
{
    tString word;
    s >> word;

    if (word.Filter() == "")
        return;

    bool found = false;
    if (se_BannedWords->Count() >= 0)
    {
        for(int i = 0; i < se_BannedWords->Count(); i++)
        {
            tString wordSearch = se_BannedWords->GetWord(i);
            if (wordSearch.Filter() == word.Filter())
            {
                found = true;
                break;
            }
        }
    }

    if (!found)
    {
        se_BannedWords->Add(word);
    }
}
static tConfItemFunc se_BannedWordsAddConf("BANNED_WORDS_ADD", &se_BannedWordsAdd);

static void se_BannedWordsRemove(std::istream &s)
{
    tString word;
    s >> word;

    if (word.Filter() == "")
        return;

    bool found = false;
    int wordId = -1;

    if (se_BannedWords->Count() >= 0)
    {
        for(int i = 0; i < se_BannedWords->Count(); i++)
        {
            tString wordSearch = se_BannedWords->GetWord(i);
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
        se_BannedWords->RemoveWord(wordId);
    }
}
static tConfItemFunc se_BannedWordsRemoveConf("BANNED_WORDS_REMOVE", &se_BannedWordsRemove);

static void se_BannedWordsList(std::istream &s)
{
    int max = 10;
    int showing = 0;

    tString params;
    int pos = 0;
    params.ReadLine(s);

    int showAmount = 0;
    tString amount = params.ExtractNonBlankSubString(pos);
    if (amount.Filter() != "") showAmount = atoi(amount);

    if (se_BannedWords->Count() > 0)
    {
        if (showAmount < se_BannedWords->Count())
        {
            if (se_BannedWords->Count() < max) max = se_BannedWords->Count();
            if ((se_BannedWords->Count() - showAmount) < max) max = se_BannedWords->Count() - showAmount;

            if (max > 0)
            {
                sn_ConsoleOut(tOutput("$banned_words_list"), 0);

                for(int i = 0; i < max; i++)
                {
                    int rotID = showAmount + i;
                    tString word = se_BannedWords->GetWord(i);
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
                show.SetTemplateParameter(2, se_BannedWords->Count());
                show << "$banned_words_list_show";
                sn_ConsoleOut(show, 0);
            }
        }
    }
}
static tConfItemFunc se_BannedWordsListConf("BANNED_WORDS_LIST", &se_BannedWordsList);
static tAccessLevelSetter se_BannedWordsListConfLevel( se_BannedWordsListConf, tAccessLevel_Moderator );

static int se_BannedWordsOptions = 0;
bool restrictBannedWordsOptionsValue(const int &newValue)
{
    if ((newValue < 0) || (newValue > 2))
        return false;

    return true;
}
static tSettingItem<int> se_BannedWordsOptionsConf("BANNED_WORDS_OPTIONS", se_BannedWordsOptions, &restrictBannedWordsOptionsValue);

bool eBannedWords::BadWordTrigger(ePlayerNetID *sender, tString &message)
{
    if ((se_BannedWords->Count() > 0) && (se_BannedWordsOptions > 0))
    {
        //  Loop through each bad word container and check in message if that bad word exists
        for (int wordID = 0; wordID < se_BannedWords->Count(); wordID++)
        {
            //  fetch the word currently in wordID
            tString word = se_BannedWords->BannedWordsList()[wordID];

            //  check if a banned word exists in the message
            if ((word.Filter() != "") && (message.Contains(word)))
            {
                //  option 1: alert the sender of the usage of banned word in their message
                if (se_BannedWordsOptions == 1)
                {
                    tOutput msg;
                    msg << "$banned_words_warning";
                    sn_ConsoleOut(msg, sender->Owner());

                    return true;
                }
                //  option 2: replace the banned word with the replacement word from the language setting
                else if (se_BannedWordsOptions == 2)
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
