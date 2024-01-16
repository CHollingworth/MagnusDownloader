#include <iostream>
#include "pugixml.hpp"
#include <string>
#include <curl/curl.h>
#include <regex>
#include "filesystem"

/**
 * @brief Struct to store episode information.
 */
struct EpisodeInfo {
    std::string name;          /**< Episode title. */
    std::string link;          /**< Episode link. */
    int episodeNumber;         /**< Episode number. */
};

/**
 * @brief Parses the RSS feed XML data using a regex pattern.
 *
 * @param pattern The regex pattern to extract episode numbers.
 * @param xmlData The XML data of the RSS feed.
 * @param episodes Vector to store parsed EpisodeInfo objects.
 */
void parseRssFeed(std::regex pattern, const std::string& xmlData, std::vector<EpisodeInfo>& episodes) {
    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_string(xmlData.c_str());

    if (!result) {
        std::cerr << "Error parsing XML: " << result.description() << std::endl;
        return;
    }

    for (const auto& item : doc.select_nodes("/rss/channel/item")) {
        EpisodeInfo episode;

        episode.name = item.node().child("title").text().get();
        episode.link = item.node().child("enclosure").attribute("url").value();

        std::smatch match;
        if (std::regex_search(episode.name, match, pattern)) {
            episode.episodeNumber = std::stoi(match[1].str());
        } else {
            episode.episodeNumber = -1;
        }
        if (episode.episodeNumber != -1) {
            episodes.push_back(episode);
        }
    }
}

/**
 * @brief Sets the track number of an episode in the MP3 file using id3v2.
 *
 * @param filePath The path to the MP3 file.
 * @param episode Episode information.
 */
void setTrackNumber(const std::string& filePath, EpisodeInfo episode) {
    std::string command = "id3v2 --track " + std::to_string(episode.episodeNumber) + " --song \""+episode.name+"\" \"" + filePath + "\"";
    int result = std::system(command.c_str());

    if (result != 0) {
        std::cerr << "Error setting track number." << std::endl;
    } else {
        std::cout << "Track number set successfully." << std::endl;
    }
}

/**
 * @brief Downloads the content of an episode using libcurl.
 *
 * @param episode Episode information.
 */
void downloadContent(EpisodeInfo episode) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        std::cerr << "Error initializing curl for download." << std::endl;
        return;
    }

    // Set the URL for curl
    curl_easy_setopt(curl, CURLOPT_URL, episode.link.c_str());

    // Set the callback function to write the response to a file or process it as needed
    // Replace FILE_PATH with the path where you want to save the downloaded content
    FILE* file = fopen(("Downloads/"+episode.name+".mp3").c_str(), "wb");
    if (!file) {
        std::cerr << "Error opening file for writing." << std::endl;
        curl_easy_cleanup(curl);
        return;
    }

    curl_easy_setopt(curl, CURLOPT_WRITEDATA, file);

    // Perform the curl request for downloading
    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        std::cerr << "curl_easy_perform() failed for download: " << curl_easy_strerror(res) << std::endl;
    }

    // Clean up curl and close the file
    curl_easy_cleanup(curl);
    fclose(file);

    setTrackNumber("Downloads/"+episode.name+".mp3", episode);
}

/**
 * @brief Callback function to write downloaded data to a string.
 *
 * @param contents Data received from the curl request.
 * @param size Size of each data element.
 * @param nmemb Number of data elements.
 * @param output String to store the downloaded data.
 * @return Total size of the written data.
 */
size_t writeCallback(void* contents, size_t size, size_t nmemb, std::string* output) {
    size_t totalSize = size * nmemb;
    output->append(static_cast<char*>(contents), totalSize);
    return totalSize;
}

int main(int argc, char* argv[]) {
    // Command line argument check
    if (!argv[1]) {
        std::cerr << "Add your patreon URL" << std::endl;
        return 1;
    }

    // Extract URL from command line argument
    std::cout << argv[1] << std::endl;
    std::string url = argv[1];

    // Create 'Downloads' directory if it doesn't exist
    std::filesystem::create_directories("Downloads");

    // Initialize curl
    CURL* curl = curl_easy_init();
    if (!curl) {
        std::cerr << "Error initializing curl." << std::endl;
        return 1;
    }

    // Set the URL for curl
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

    // Set the callback function to write the response
    std::string responseData;
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseData);

    // Perform the curl request to retrieve RSS feed data
    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
        curl_easy_cleanup(curl);
        return 1;
    }

    curl_easy_cleanup(curl);

    // Parse RSS feed for MAG episodes
    std::vector<EpisodeInfo> episodes;
    std::regex pattern("MAG (\\d+)", std::regex::icase);
    parseRssFeed(pattern, responseData, episodes);

    // Sort MAG episodes by episode number
    std::sort(episodes.begin(), episodes.end(), [](const EpisodeInfo& a, const EpisodeInfo& b) {
        return a.episodeNumber < b.episodeNumber;
    });

    // Display and download MAG episodes
    for (const auto& episode : episodes) {
        std::cout << "Title: " << episode.name << std::endl;
        std::cout << "Link: " << episode.link << std::endl;
        std::cout << "Episode Number: " << episode.episodeNumber << std::endl;
        downloadContent(episode);
        std::cout << "----------------------" << std::endl;
    }

    // Parse RSS feed for The Magnus Protocol episodes
    std::vector<EpisodeInfo> episodes2;
    std::regex pattern2("The Magnus Protocol (\\d+)", std::regex::icase);
    parseRssFeed(pattern2, responseData, episodes2);

    // Sort The Magnus Protocol episodes by episode number
    std::sort(episodes2.begin(), episodes2.end(), [](const EpisodeInfo& a, const EpisodeInfo& b) {
        return a.episodeNumber < b.episodeNumber;
    });

    // Display and download The Magnus Protocol episodes
    for (const auto& episode : episodes2) {
        std::cout << "Title: " << episode.name << std::endl;
        std::cout << "Link: " << episode.link << std::endl;
        std::cout << "Episode Number: " << episode.episodeNumber << std::endl;
        downloadContent(episode);
        std::cout << "----------------------" << std::endl;
    }

    return 0;
}