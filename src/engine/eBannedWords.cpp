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
                    tString word = se_BannedWords->GetWord(rotID);
                    if (word.Filter() != "")
                    {
                        tColoredString send;
                        send << tColoredString::ColorString( 1,1,.5 );
                        send << "( ";
                        send << "0xaacc09" << word;
                        send << tColoredString::ColorString( 1,1,.5 );
                        if (i + 1 == max)
                            send << " )\n";
                        else
                            send << " ),\n";
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

tString se_BannedWordsDelimiters("`~!@#$%^&*()-=_+[] \{}|;':\",./<>?");
static tSettingItem<tString> se_BannedWordsDelimitersConf( "BANNED_WORDS_DELIMITERS", se_BannedWordsDelimiters);

bool eBannedWords::CharacterInDelimiter(char character)
{
    for(int i = 0; i < (se_BannedWordsDelimiters.Len()); i++)
    {
        if (se_BannedWordsDelimiters[i] == character)
            return true;
    }

    return false;
}

bool eBannedWords::HasBadWord(tString message, tString word)
{
    tString origLowMsg(message.ToLower());

    if (tColoredString::HasColors(origLowMsg))
        origLowMsg = tColoredString::RemoveColors(origLowMsg);

    //  check if word exists in converted message
    int pos = 0;

    for(int i = 0; i < origLowMsg.Len(); i++)
    {
        if (CharacterInDelimiter(origLowMsg[i]))
            origLowMsg = origLowMsg.RemoveCharacter(origLowMsg[i]);
    }

    //  set words lower case and check if word contains in gotWord
    if (origLowMsg.ToLower().Contains(word.ToLower()))
        return true;

    return false;
}

bool eBannedWords::BadWordTrigger(ePlayerNetID *sender, tString &message)
{
    if ((se_BannedWords->Count() > 0) && (se_BannedWordsOptions > 0))
    {
        //  Loop through each bad word container and check in message if that bad word exists
        for (int wordID = 0; wordID < se_BannedWords->Count(); wordID++)
        {
            //  fetch the word currently in wordID
            tString word = se_BannedWords->GetWord(wordID);

            //  check if a banned word exists in the message
            if ((word.Filter() != "") && HasBadWord(message, word))
            {
                //  option 1: alert the sender of the usage of banned word in their message
                if (se_BannedWordsOptions == 1)
                {
                    //  build the message to warn the sender
                    tOutput msg;
                    msg << "$banned_words_warning";
                    sn_ConsoleOut(msg, sender->Owner());

                    return true;
                }
                //  option 2: replace the banned word with the replacement word from the language setting
                else if (se_BannedWordsOptions == 2)
                {
                    //  switch the bad word with the replacement character(s)
                    message = ReplaceBadWords(message, word);
                }
            }
        }
    }

    return false;
}

bool eBannedWords::BadWordTrigger(tString &message)
{
    if ((se_BannedWords->Count() > 0) && (se_BannedWordsOptions > 0))
    {
        //  Loop through each bad word container and check in message if that bad word exists
        for (int wordID = 0; wordID < se_BannedWords->Count(); wordID++)
        {
            //  fetch the word currently in wordID
            tString word = se_BannedWords->GetWord(wordID);

            //  check if a banned word exists in the message
            if ((word.Filter() != "") && HasBadWord(message, word))
            {
                if (se_BannedWordsOptions == 2)
                {
                    //  switch the bad word with the replacement character(s)
                    message = ReplaceBadWords(message, word);
                }
            }
        }
    }

    return false;
}

static bool se_BannedWordsWhole = true;
static tSettingItem<bool> se_BannedWordsWholeConf("BANNED_WORDS_WHOLE", se_BannedWordsWhole);

tString eBannedWords::ReplaceBadWords(tString message, tString word)
{
    tString originalMsg(message);
    tString convertedMsg;
    tString replacement(tOutput("$banned_words_replace"));

    if (originalMsg.Filter() != "")
    {
        tArray<tString> splitWords = originalMsg.Split(" ");
        for(int i = 0; i < splitWords.Len(); i++)
        {
            tString splitWord = splitWords[i];
            if (splitWord == "")
            {
                if ((i + 1) < splitWords.Len())
                    convertedMsg << " ";

                continue;
            }

            tString splitWordCon(splitWord);

            for(int j = 0; j < splitWordCon.Len(); j++)
            {
                if (CharacterInDelimiter(splitWordCon[j]))
                    splitWordCon = splitWordCon.RemoveCharacter(splitWordCon[j]);
            }

            //  check if it has colours in its text
            if (tColoredString::HasColors(splitWordCon))
            {
                //  if so, filter them out
                splitWordCon = tColoredString::RemoveColors(splitWordCon);
            }

            //  do the censorship
            if (splitWordCon.ToLower() == word.ToLower())
            {
                tString replaced;

                if (se_BannedWordsWhole)
                {
                    for(int k = 0; k < (splitWordCon.Len() - 1); k++)
                        replaced << replacement;
                }
                else
                {
                    tString f_letter = splitWordCon.SubStr(0, 1);
                    tString l_letter = splitWordCon.SubStr(splitWordCon.Len() - 2, 1);

                    replaced = f_letter << replacement << l_letter;
                }

                convertedMsg << replaced << " ";
            }
            //  otherwise, append the normal text
            else
            {
                if ((i + 1) == splitWords.Len())
                    convertedMsg << splitWord;
                else
                    convertedMsg << splitWord << " ";
            }
        }
    }

    return convertedMsg;
}
