#ifndef CONSTANTS_H
#define CONSTANTS_H
#include <QString>

// Google Speech-to-Text API
static QString speechBaseApi = "https://speech.googleapis.com/v1/speech:recognize";
static QString speechApiKey = "your-google-speech-api-key";

// Translation API (MyMemory via RapidAPI)
static QString translateUrl = "https://translated-mymemory---translation-memory.p.rapidapi.com/get";
static QString translateHost = "translated-mymemory---translation-memory.p.rapidapi.com";
static QString translateApiKey = "your-rapidapi-key";

// Google Text-to-Speech API
const QString googleTtsBaseApi = "https://texttospeech.googleapis.com/v1/text:synthesize";
const QString googleTtsApiKey = "your-google-tts-api-key";

// Claude (Anthropic) API
const QString claudeApiKey = "your-anthropic-api-key";

#endif // CONSTANTS_H
