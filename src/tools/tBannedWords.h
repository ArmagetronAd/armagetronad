#ifndef TBANNEDWORD_H
#define TBANNEDWORD_H

#include "tString.h"
#include "tArray.h"
#include "tList.h"
#include "ePlayer.h"

class tBannedWords
{
    public:
        tBannedWords();

        tArray<tString> BannedWordsList() { bannedWords_; }

        void Add(tString word) { bannedWords_.Insert(word); }

        tString GetWord(int id)
        {
            if ((id >= 0) && (id < bannedWords_.Len()))
            {
                return bannedWords_[id];
            }

            return tString("");
        }

        void RemoveWord(int id)
        {
            if ((id >= 0) && (id < bannedWords_.Len()))
            {
                bannedWords_.RemoveAt(id);
            }
        }

        void Clear()
        {
            /*if (bannedWords_.Len() > 0)
            {
                for(int i = 0; i < bannedWords_.Len(); i++)
                {
                    bannedWords_.RemoveAt(i);
                    i--;
                }
            }

            bannedWords_.RemoveAt(0);*/
            bannedWords_.Clear();
            bannedWords_.SetLen(0);
        }

        int Count() { bannedWords_.Len(); }

        static bool BadWordTrigger(ePlayerNetID *sender, tString &message);

    private:
        tArray<tString> bannedWords_;
};

extern tBannedWords *st_BannedWords;

#endif // TBANNEDWORD_H
