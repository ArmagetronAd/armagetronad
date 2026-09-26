# How To Contribute

THIS IS A FIRST DRAFT, improvement suggestions welcome.

If you disagree with any of that, say so. If we end up disagreeing, that is also fine; if the disagreement is important enough to you, you can always fork, no hard feelings from us.

## Code of Conduct

Let's keep this short.
 1. Try not to be an asshole.
 2. Everyone, no matter where they are coming from, should feel welcome to contribute.
 3. Respect the rules of your chosen communication platform. On Discord, read the #rules channel.
 4. Enter discussions in good faith, and **start** by assuming the other people are doing the same. If you find they are not, it's best to not waste your time and just walk away.
 5. We prefer honesty to politeness. If something is a fucking stupid idea, you may say so. But say "This is a fucking stupid idea", not "You are a fucking stupid idiot for bringing this up, do you have jelly betwen your ears or what?".
 6. Show no Racism, Sexism, Xenophobia, Homophobia, Transphobia, you know the stuff. It's what makes people think "Ouch, this place looks like a Nazi Bar, do I want to hang out there? I think not."

## You Should Know

Be aware that this project is under GPL, which means every change
you publish anywhere automatically also is under GPL. This happens rarely, but:
We can, if we so choose, simply grab your stuff and integrate it even if you do not submit it.
Giving you proper credit, of course.

We don't do code ownership. Everyone is entitled to improve your code, as you are entitled to improve ours.

## Before You Begin

If you have a technical fix, there is nothing to do, just fix ahead.

For other changes, go float your change idea to the community. 
Maybe someone else is already working in it, 
maybe the direction you want to go in conflicts with something. 
You can potentially save yourself a lot of work by just asking, 
and maybe there are useful tips in it for you on how to best achieve your goal.
The best current place is [Discord](https://discord.gg/dcpaauj), 
the armagetron-dev channel.

## Work

Do just one thing per submission. 
Not 'fix translation, fix a crash, implement flying cylces' in one go! 
We might only want to accept a subset of those, 
and if they are all lumped together, we need to untangle them.

One excception: If the things are similar and it's clear we will either accept them all or none and if we want to cherry pick, that's our problem,
smash them together. 
Ten crashfixes in one submission are fine. Fixing all translation errors you can find in one of the languages is fine.

## Coding Style

Don't worry too much about formatting. We have `.editorconfig` and `.clang-format` files that a decent code editor will respect. 

The coding style is provided as verbosely commented samples in `src/test/CodingStyle*`. 
If it's not in there, it's not terribly important. Not everythin IN there is terribly important. Really important:
 - indentation: 4 spaces
 - curly braces on their own lines
 - in general, classes and methods are CamelCase, variables camelCase, member variables with a _ at the end

## Submitting

Work is best done by forking our git repository on GitLab, working on a branch on your fork, then submitting a Merge Request.
Pull Requests on GitHub are also fine.

Old school contributions via patch files also work, but they are harder to discuss, so they are discouraged.

## The Pragmatic AI Manifesto

### Introduction

**This is a Humans First project**. Humans should always be able to read, understand, maintain and extend the source code.

However, there is no denying that the various AI tools can be useful, and many of want to actively use them. We are not fundamentally anti-AI, only
aware of the adverse effects they can have. We are doing what we can to make work with these tools possible and safe.

There have to be some rules, though, to avoid sloppification.

### Hard Passes

Do not use AI generators to:
 - Make Images, Sound, Music. Many reasons, but the easiest to point out is that there seems to be a general consensus among gamers: They hate that stuff.
 - Write texts to be read by humans, gamers and developers alike. Write your documentation yourself. Exceptions:
   - AI grammar and spell checkers are fine.
   - Code comments and commit messages written along with the code are fine.
   - Translation tools are fine when communicating on forums and chats. They're not fine for generating translation texts for the game, or translating documentation.

### Code Generation and Modification

We do allow AI tools for coding tasks, with these rules:
 - Disclose your use.
 - No pure vibe coding. You are ultimately responsible for the code you write. Read the output, understand it, fix it where it is lacking (or tell the AI to fix it). Only then can you be sure other humans can understand it, too.
 - We have unit tests, make use of them. AI generated or modified code should be covered by tests whenever possible.
 - If the systems you want to work on do not yet have tests, your first job,
 before doing anything else, is to create tests that document the status quo. That way, you can check that your changes break nothing, or at least break only the things they are meant to break.
 - For non-trivial changes, if you can, use a specify-plan-execute harness that guides the AI.

