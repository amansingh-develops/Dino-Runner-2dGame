#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <iostream>
#include <array>
#include <vector>
#include <random>

const unsigned int windowSize_x = 1000;
const unsigned int windowSize_y = 500;
const unsigned int groundLevel = 465; // Y position where dino stands (bottom of screen minus small offset)
const unsigned int groundOffset = groundLevel; // Keep compatibility
int gameSpeed = 8;
bool playerDead = false;
bool playDeadSound = false;

enum PowerUpType { SHIELD, INVINCIBILITY, SPEED_BOOST };

struct PowerUp
{
    sf::CircleShape shape;
    PowerUpType type;
    sf::Vector2f position;
    
    PowerUp(float x, float y, PowerUpType t)
    :type(t)
    {
        shape.setRadius(15.f);
        position = sf::Vector2f(x, y);
        shape.setPosition(position);
        
        if(type == SHIELD)
            shape.setFillColor(sf::Color::Blue);
        else if(type == INVINCIBILITY)
            shape.setFillColor(sf::Color::Yellow);
        else
            shape.setFillColor(sf::Color::Red);
    }
};

struct Fps_s
{
    sf::Font font;
    sf::Text text;
    sf::Clock clock;
    int Frame = 0;
    int fps = 0;
    
    Fps_s() : text(font) {}
};
class Fps
{
    Fps_s fps;
    public:
        Fps()
        {
            if(fps.font.openFromFile("rsrc/Fonts/font.ttf"))
            {
                fps.text.setFont(fps.font);
            }
            fps.text.setCharacterSize(15);
            fps.text.setPosition(sf::Vector2f(fps.text.getCharacterSize() + 10.f, fps.text.getCharacterSize()));
            fps.text.setFillColor(sf::Color(83, 83, 83));
        }
        void update()
        {
            if(fps.clock.getElapsedTime().asSeconds() >= 1.f)
            {
                fps.fps = fps.Frame; fps.Frame = 0; fps.clock.restart();
            }
            fps.Frame++;
            fps.text.setString("FPS :- " + std::to_string(fps.fps));
        }
        void drawTo(sf::RenderWindow& window)
        {
            window.draw(fps.text);
        }

};


class SoundManager
{
    public:
        sf::SoundBuffer dieBuffer;
        sf::SoundBuffer jumpBuffer;
        sf::SoundBuffer pointBuffer;
        sf::Sound dieSound;
        sf::Sound jumpSound;
        sf::Sound pointSound;

        SoundManager()
        :dieSound(dieBuffer), jumpSound(jumpBuffer), pointSound(pointBuffer)
        {
            if(!dieBuffer.loadFromFile("rsrc/Sounds/die.wav")) std::cout << "Error loading die sound\n";
            if(!jumpBuffer.loadFromFile("rsrc/Sounds/jump.wav")) std::cout << "Error loading jump sound\n";
            if(!pointBuffer.loadFromFile("rsrc/Sounds/point.wav")) std::cout << "Error loading point sound\n";
        }
};

class Ground
{
    public:
    sf::Texture groundTexture;
    sf::Sprite groundSprite;
    int offset{0};
    Ground() : groundSprite(groundTexture)
    {
        if(groundTexture.loadFromFile("rsrc/Images/GroundImage.png"))
        {
            groundSprite.setTexture(groundTexture);
            groundSprite.setPosition(sf::Vector2f(0.f, windowSize_y - groundTexture.getSize().y - 50));
        }
    }   

    void updateGround()
    {
        if(playerDead == false)
        {
            if(offset > (int)groundTexture.getSize().x - (int)windowSize_x)
                offset = 0;

            offset += gameSpeed;
            groundSprite.setTextureRect(sf::IntRect(sf::Vector2i(offset, 0), sf::Vector2i(windowSize_x, windowSize_y)));
        }

        if(playerDead == true)
            groundSprite.setTextureRect(sf::IntRect(sf::Vector2i(offset, 0), sf::Vector2i(windowSize_x, windowSize_y)));
        
    }
    void reset()
    {
        offset = 0;
        groundSprite.setTextureRect(sf::IntRect(sf::Vector2i(0, 0), sf::Vector2i(windowSize_x, windowSize_y)));
    }

};


// Forward declaration
class Dino;

class Obstacle
{
    public:
        sf::Sprite obstacleSprite;
        sf::FloatRect obstacleBounds;
        Obstacle(sf::Texture& texture)
        :obstacleSprite(texture), obstacleBounds(sf::Vector2f(0.f, 0.f), sf::Vector2f(0.f, 0.f))
        {
            // Set origin to bottom-left for proper ground alignment
            obstacleSprite.setOrigin(sf::Vector2f(0.f, texture.getSize().y));
            obstacleSprite.setPosition(sf::Vector2f(windowSize_x, groundLevel));
        }
};

class Obstacles
{
    public:
        std::vector<Obstacle> obstacles;
        std::vector<PowerUp> powerUps;
        std::vector<sf::RectangleShape> meteorites;
        
        sf::Time spawnTimer;
        sf::Time powerUpTimer;
        sf::Time meteoriteTimer;
        sf::Time lastObstacleTime;
        sf::Texture obstacleTexture_1;
        sf::Texture obstacleTexture_2;
        sf::Texture obstacleTexture_3; 
        int randomNumber{0};
        float minSafeDistance{300.f};

        Obstacles()
        :spawnTimer(sf::Time::Zero), powerUpTimer(sf::Time::Zero), meteoriteTimer(sf::Time::Zero), lastObstacleTime(sf::Time::Zero)
        {
            obstacles.reserve(5);
            powerUps.reserve(3);
            meteorites.reserve(5);
            
            if(obstacleTexture_1.loadFromFile("rsrc/Images/Cactus1.png"))
            {
                std::cout << "loaded cactus Image 1 " << std::endl;
            }

            if(obstacleTexture_2.loadFromFile("rsrc/Images/Cactus2.png"))
            {
                std::cout << "Loaded cactus Image 2" << std::endl;
            }

            if(obstacleTexture_3.loadFromFile("rsrc/Images/Cactus3.png"))
            {
                std::cout << "Loaded cactus Image 3" << std::endl;
            }
        }

        bool isSafeToSpawn()
        {
            for(auto& meteorite : meteorites)
            {
                float meteoriteX = meteorite.getPosition().x;
                float meteoriteY = meteorite.getPosition().y;
                
                if(meteoriteX > windowSize_x - 200.f && meteoriteX < windowSize_x + 300.f && meteoriteY < groundLevel - 50.f)
                {
                    return false;
                }
            }
            return true;
        }

        void update(sf::Time& deltaTime, Dino& dino);

        void drawTo(sf::RenderWindow& window)
        {
            for(auto& obs : obstacles)
            {
                window.draw(obs.obstacleSprite);
            }
            for(auto& powerUp : powerUps)
            {
                window.draw(powerUp.shape);
            }
            for(auto& meteorite : meteorites)
            {
                window.draw(meteorite);
            }
        }

        void reset()
        {
            obstacles.erase(obstacles.begin(), obstacles.end());
            powerUps.erase(powerUps.begin(), powerUps.end());
            meteorites.erase(meteorites.begin(), meteorites.end());
            lastObstacleTime = sf::Time::Zero;
        }
};



class Dino
{
    public:
        sf::Sprite dino;
        sf::Vector2f dinoPos{0.f, 0.f};
        sf::Vector2f dinoMotion{0.f, 0.f};
        sf::Texture dinoTex;
        sf::FloatRect dinoBounds;
        SoundManager soundManager;
        std::array<sf::IntRect, 6> frames;
        sf::Time timeTracker;
        int animationCounter{0};
        bool isDucking{false};
        float normalHeight{95.f};
        float duckHeight{50.f};
        
        // Power-up states
        bool hasShield{false};
        bool isInvincible{false};
        sf::Time invincibilityTimer;
        sf::Time shieldTimer;
        float invincibilityDuration{5.f};
        float shieldDuration{8.f};
        
        // Size growth mechanic
        float currentScale{1.0f};
        float minScale{1.0f};
        float maxScale{1.5f};
        bool isGrowing{true};
        
        // Sound control
        bool deathSoundPlayed{false};
        
        Dino()
        :dino(dinoTex), soundManager()
        {
            if(dinoTex.loadFromFile("rsrc/Images/PlayerSpriteSheet.png"))
            {
                dino.setTexture(dinoTex);
                for(int i = 0; i < frames.size(); i++)
                    frames[i] = sf::IntRect(sf::Vector2i(i * 90, 0), sf::Vector2i(90, 95));
                dino.setTextureRect(frames[0]);
                // Set origin to bottom-center for proper ground alignment
                dino.setOrigin(sf::Vector2f(45.f, 95.f)); // Bottom center, not middle
                dinoPos = dino.getPosition();
            }
            else
            {
                std::cout << "Error loading the PlayerSprite texture" << std::endl;
            }
        }

        void update(sf::Time& deltaTime, std::vector<Obstacle>& obstacles)
        {
            dinoPos = dino.getPosition();
            dinoBounds = dino.getGlobalBounds();
            dinoBounds.size.y -= 15.f * currentScale;
            dinoBounds.size.x -= 10.f * currentScale;
            timeTracker += deltaTime;
            
            dino.setScale(sf::Vector2f(currentScale, currentScale));
            
            if(isInvincible)
            {
                invincibilityTimer += deltaTime;
                if(invincibilityTimer.asSeconds() > invincibilityDuration)
                    isInvincible = false;
            }
            
            if(hasShield)
            {
                shieldTimer += deltaTime;
                if(shieldTimer.asSeconds() > shieldDuration)
                    hasShield = false;
            }
            
            for(auto& obs: obstacles)
            {
                if(dinoBounds.findIntersection(obs.obstacleBounds))
                {
                    if(isInvincible)
                    {
                        soundManager.jumpSound.play();
                    }
                    else if(hasShield)
                    {
                        hasShield = false;
                        soundManager.jumpSound.play();
                    }
                    else
                    {
                        playerDead = true;
                    }
                }
            }

            if(!playerDead)
            {
                if(sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down) && dinoPos.y >= groundLevel)
                {
                    isDucking = true;
                    dino.setTextureRect(sf::IntRect(sf::Vector2i(0, 95), sf::Vector2i(110, duckHeight)));
                    dino.setPosition(sf::Vector2f(dinoPos.x, groundLevel + (normalHeight - duckHeight)));
                }
                else
                {
                    isDucking = false;
                    walk();
                    if(sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Space) == true && dinoPos.y >= groundLevel)
                    {
                        animationCounter = 0;
                        dinoMotion.y = -20.f; 
                        dino.setTextureRect(frames[1]);
                        soundManager.jumpSound.play();
                    }

                    if(dinoPos.y < groundLevel)
                    {
                        dinoMotion.y += 1.f; 
                        dino.setTextureRect(frames[1]);
                    }

                    if(dinoPos.y > groundLevel)
                    {
                        dino.setPosition(sf::Vector2f(dino.getPosition().x, groundLevel));
                        dinoMotion.y = 0.f;
                    }

                    dino.move(dinoMotion);
                }
            } 
            if(playerDead == true)
            {
                dinoMotion.y = 0.f;
                dino.setTextureRect(frames[3]);
                
                if(!deathSoundPlayed)
                {
                    soundManager.dieSound.play();
                    deathSoundPlayed = true;
                }
            }

        }

        void walk()
        {
            for(int i = 0; i < frames.size() - 3; i++)
                if(animationCounter == (i + 1) * 3)
                    dino.setTextureRect(frames[i]);

            if(animationCounter >= (frames.size() - 2) * 3)
                animationCounter = 0;

            animationCounter++;
        }
        void reset()
        {
            dinoMotion.y = 0; 
            dino.setPosition(sf::Vector2f(dino.getPosition().x, groundLevel));
            dino.setTextureRect(frames[0]);
            hasShield = false;
            isInvincible = false;
            invincibilityTimer = sf::Time::Zero;
            shieldTimer = sf::Time::Zero;
            currentScale = 1.0f;
            isGrowing = true;
            dino.setScale(sf::Vector2f(1.0f, 1.0f));
            deathSoundPlayed = false;
        }

}; 

void Obstacles::update(sf::Time& deltaTime, Dino& dino)
{
    spawnTimer += deltaTime;
    powerUpTimer += deltaTime;
    meteoriteTimer += deltaTime;
    lastObstacleTime += deltaTime;
    
    if(spawnTimer.asSeconds() > 1.5f + gameSpeed/8 && isSafeToSpawn())
    {
        randomNumber = (rand() % 3) + 1;
        if(randomNumber == 1)
        {
            obstacles.emplace_back(Obstacle(obstacleTexture_1));
        }
        if(randomNumber == 2)
        {
            obstacles.emplace_back(Obstacle(obstacleTexture_2));
        }
        if(randomNumber == 3)
        {
            obstacles.emplace_back(Obstacle(obstacleTexture_2));
        }
        spawnTimer = sf::Time::Zero;
        lastObstacleTime = sf::Time::Zero;
    }
    
    if(powerUpTimer.asSeconds() > 10.f + gameSpeed/3)
    {
        int powerType = rand() % 3;
        float yPosition = groundLevel - 80.f - (rand() % 40);
        powerUps.emplace_back(PowerUp(windowSize_x + 50.f, yPosition, (PowerUpType)powerType));
        powerUpTimer = sf::Time::Zero;
    }
    
    if(meteoriteTimer.asSeconds() > 10.f + gameSpeed/4 && lastObstacleTime.asSeconds() > 2.f)
    {
        sf::RectangleShape meteorite(sf::Vector2f(25.f, 35.f));
        meteorite.setFillColor(sf::Color(150, 75, 0));
        float xOffset = 100.f + rand() % 150;
        meteorite.setPosition(sf::Vector2f(windowSize_x + xOffset, 50.f));
        meteorites.emplace_back(meteorite);
        meteoriteTimer = sf::Time::Zero;
    }

    if(playerDead == false)
    {
        for(int i = 0; i < obstacles.size(); i++)
        {
            obstacles[i].obstacleBounds = obstacles[i].obstacleSprite.getGlobalBounds();
            obstacles[i].obstacleBounds.size.x -= 10.f;
            obstacles[i].obstacleSprite.move(sf::Vector2f(-1*gameSpeed, 0.f));
            if(obstacles[i].obstacleSprite.getPosition().x < -150.f)
            {
                obstacles.erase(obstacles.begin() + i);
            }
        }
        
        for(int i = 0; i < powerUps.size(); i++)
        {
            powerUps[i].shape.move(sf::Vector2f(-1*gameSpeed, 0.f));
            
            sf::FloatRect powerUpBounds = powerUps[i].shape.getGlobalBounds();
            if(dino.dinoBounds.findIntersection(powerUpBounds))
            {
                if(powerUps[i].type == SHIELD)
                {
                    dino.hasShield = true;
                    dino.shieldTimer = sf::Time::Zero;
                }
                else if(powerUps[i].type == INVINCIBILITY)
                {
                    dino.isInvincible = true;
                    dino.invincibilityTimer = sf::Time::Zero;
                }
                else if(powerUps[i].type == SPEED_BOOST)
                {
                    gameSpeed += 2;
                }
                powerUps.erase(powerUps.begin() + i);
                dino.soundManager.pointSound.play();
            }
            else if(powerUps[i].shape.getPosition().x < -50.f)
            {
                powerUps.erase(powerUps.begin() + i);
            }
        }
        
        for(int i = 0; i < meteorites.size(); i++)
        {
            meteorites[i].move(sf::Vector2f(-1*gameSpeed, 2.5f));
            
            sf::FloatRect meteoriteBounds = meteorites[i].getGlobalBounds();
            if(dino.dinoBounds.findIntersection(meteoriteBounds))
            {
                if(dino.isGrowing)
                {
                    dino.currentScale += 0.05f;
                    if(dino.currentScale >= dino.maxScale)
                    {
                        dino.currentScale = dino.maxScale;
                        dino.isGrowing = false;
                    }
                }
                else
                {
                    dino.currentScale -= 0.025f;
                    if(dino.currentScale <= dino.minScale)
                    {
                        dino.currentScale = dino.minScale;
                        dino.isGrowing = true;
                    }
                }
                
                dino.soundManager.jumpSound.play();
                meteorites.erase(meteorites.begin() + i);
            }
            else if(meteorites[i].getPosition().y > windowSize_y)
            {
                meteorites.erase(meteorites.begin() + i);
            }
        }
    }
}



class Scores
{
    public:
        sf::Font scoresFont;
        sf::Text previousScoreText;
        sf::Text HIText;
        sf::Text scoresText;
        sf::Text difficultyText;
        sf::Text speedText;
        sf::Text powerUpStatusText;
        sf::Text shieldTimerText;
        sf::Text invincibilityTimerText;
        sf::Text sizeText;
        sf::RectangleShape shieldBar;
        sf::RectangleShape invincibilityBar;
        sf::RectangleShape shieldBarBg;
        sf::RectangleShape invincibilityBarBg;
        SoundManager soundManager;
        short scores{0};
        short previousScore{0};
        short scoresIndex{0};
        short scoresDiff{0};
        short scoresInital{0};
        int difficulty{1};

        Scores()
        :previousScoreText(scoresFont), HIText(scoresFont), scoresText(scoresFont), 
         difficultyText(scoresFont), speedText(scoresFont), powerUpStatusText(scoresFont),
         shieldTimerText(scoresFont), invincibilityTimerText(scoresFont), sizeText(scoresFont)
        {
            if(scoresFont.openFromFile("rsrc/Fonts/Font.ttf"))
            {
                scoresText.setFont(scoresFont);
                scoresText.setCharacterSize(15);
                scoresText.setPosition(sf::Vector2f(windowSize_x/2 + windowSize_x/4 + 185.f, scoresText.getCharacterSize() + 10.f));
                scoresText.setFillColor(sf::Color(83, 83, 83));
                
                previousScoreText.setFont(scoresFont);
                previousScoreText.setCharacterSize(15);
                previousScoreText.setPosition(sf::Vector2f(scoresText.getPosition().x - 100.f, scoresText.getPosition().y));
                previousScoreText.setFillColor(sf::Color(83, 83, 83));

                HIText.setFont(scoresFont);
                HIText.setCharacterSize(15);
                HIText.setPosition(sf::Vector2f(previousScoreText.getPosition().x - 50.f, previousScoreText.getPosition().y));
                HIText.setFillColor(sf::Color(83, 83, 83));
                
                difficultyText.setFont(scoresFont);
                difficultyText.setCharacterSize(12);
                difficultyText.setPosition(sf::Vector2f(50.f, 50.f));
                difficultyText.setFillColor(sf::Color(83, 83, 83));
                
                speedText.setFont(scoresFont);
                speedText.setCharacterSize(12);
                speedText.setPosition(sf::Vector2f(50.f, 70.f));
                speedText.setFillColor(sf::Color(83, 83, 83));
                
                powerUpStatusText.setFont(scoresFont);
                powerUpStatusText.setCharacterSize(14);
                powerUpStatusText.setPosition(sf::Vector2f(50.f, 95.f));
                powerUpStatusText.setFillColor(sf::Color(83, 83, 83));
                
                shieldTimerText.setFont(scoresFont);
                shieldTimerText.setCharacterSize(12);
                shieldTimerText.setPosition(sf::Vector2f(210.f, 118.f));
                shieldTimerText.setFillColor(sf::Color(0, 100, 255));
                shieldTimerText.setStyle(sf::Text::Bold);
                
                invincibilityTimerText.setFont(scoresFont);
                invincibilityTimerText.setCharacterSize(12);
                invincibilityTimerText.setPosition(sf::Vector2f(210.f, 143.f));
                invincibilityTimerText.setFillColor(sf::Color(200, 150, 0));
                invincibilityTimerText.setStyle(sf::Text::Bold);
                
                sizeText.setFont(scoresFont);
                sizeText.setCharacterSize(12);
                sizeText.setPosition(sf::Vector2f(50.f, 170.f));
                sizeText.setFillColor(sf::Color(83, 83, 83));
            }
            HIText.setString("HI");
            
            shieldBarBg.setSize(sf::Vector2f(150.f, 15.f));
            shieldBarBg.setPosition(sf::Vector2f(50.f, 120.f));
            shieldBarBg.setFillColor(sf::Color(200, 200, 200));
            shieldBarBg.setOutlineThickness(1.f);
            shieldBarBg.setOutlineColor(sf::Color(100, 100, 100));
            
            shieldBar.setSize(sf::Vector2f(150.f, 15.f));
            shieldBar.setPosition(sf::Vector2f(50.f, 120.f));
            shieldBar.setFillColor(sf::Color(0, 100, 255));
            
            invincibilityBarBg.setSize(sf::Vector2f(150.f, 15.f));
            invincibilityBarBg.setPosition(sf::Vector2f(50.f, 145.f));
            invincibilityBarBg.setFillColor(sf::Color(200, 200, 200));
            invincibilityBarBg.setOutlineThickness(1.f);
            invincibilityBarBg.setOutlineColor(sf::Color(100, 100, 100));
            
            invincibilityBar.setSize(sf::Vector2f(150.f, 15.f));
            invincibilityBar.setPosition(sf::Vector2f(50.f, 145.f));
            invincibilityBar.setFillColor(sf::Color(255, 215, 0));
        }

        void update()
        {
            if(playerDead == false)
            {
                scoresIndex++;
                if(scoresIndex >= 5)
                {
                    scoresIndex = 0; 
                    scores++;
                }
                scoresDiff = scores - scoresInital;
                
                int newDifficulty = 1 + (scores / 500);
                if(newDifficulty > difficulty)
                {
                    difficulty = newDifficulty;
                    gameSpeed += 2;
                    soundManager.pointSound.play();
                }   

                scoresText.setString(std::to_string(scores));
                previousScoreText.setString(std::to_string(previousScore));
                difficultyText.setString("Level: " + std::to_string(difficulty));
                speedText.setString("Speed: " + std::to_string(gameSpeed));
            }
            
        }
        
        void updatePowerUpStatus(const Dino& dino)
        {
            std::string status = "Active Powers: ";
            bool hasPowerUp = false;
            
            if(dino.hasShield)
            {
                float remaining = dino.shieldDuration - dino.shieldTimer.asSeconds();
                status += "Shield ";
                float progress = remaining / dino.shieldDuration;
                shieldBar.setSize(sf::Vector2f(150.f * progress, 15.f));
                shieldTimerText.setString("Shield Active: " + std::to_string((int)remaining) + "s");
                hasPowerUp = true;
            }
            else
            {
                shieldBar.setSize(sf::Vector2f(0.f, 15.f));
                shieldTimerText.setString("");
            }
            
            if(dino.isInvincible)
            {
                float remaining = dino.invincibilityDuration - dino.invincibilityTimer.asSeconds();
                if(hasPowerUp) status += "| ";
                status += "Invincible";
                float progress = remaining / dino.invincibilityDuration;
                invincibilityBar.setSize(sf::Vector2f(150.f * progress, 15.f));
                invincibilityTimerText.setString("Invincible: " + std::to_string((int)remaining) + "s");
                hasPowerUp = true;
            }
            else
            {
                invincibilityBar.setSize(sf::Vector2f(0.f, 15.f));
                invincibilityTimerText.setString("");
            }
            
            if(!hasPowerUp)
            {
                status += "None";
            }
            
            powerUpStatusText.setString(status);
            
            int sizePercent = (int)(dino.currentScale * 100);
            std::string sizeStatus = "Size: " + std::to_string(sizePercent) + "%";
            if(dino.isGrowing && dino.currentScale > 1.0f)
            {
                sizeStatus += " (Growing)";
            }
            else if(!dino.isGrowing && dino.currentScale > 1.0f)
            {
                sizeStatus += " (Shrinking)";
            }
            sizeText.setString(sizeStatus);
        }
        
        void drawPowerUpBars(sf::RenderWindow& window, const Dino& dino)
        {
            if(dino.hasShield)
            {
                window.draw(shieldBarBg);
                window.draw(shieldBar);
                window.draw(shieldTimerText);
            }
            if(dino.isInvincible)
            {
                window.draw(invincibilityBarBg);
                window.draw(invincibilityBar);
                window.draw(invincibilityTimerText);
            }
        }

        void reset()
        {
            if(scores > previousScore)
                previousScore = scores;
            if(scores < previousScore)
                previousScore = previousScore;
            
            previousScoreText.setString(std::to_string(previousScore));
            scores = 0;
            difficulty = 1;
        }   

};

class RestartButton
{
    public:
        sf::Texture restartButtonTexture;
        sf::Sprite restartButtonSprite;
        sf::FloatRect restartButtonSpriteBounds;
        sf::Vector2f mousePos;
        bool checkPressed{false};

        RestartButton()
        :restartButtonSprite(restartButtonTexture), mousePos(0.f, 0.f)
        {
            if(restartButtonTexture.loadFromFile("rsrc/Images/RestartButton.png"))
            {
                restartButtonSprite.setTexture(restartButtonTexture);
                restartButtonSprite.setPosition(sf::Vector2f(windowSize_x/2 - restartButtonTexture.getSize().x/2, windowSize_y/2));
                restartButtonSpriteBounds = restartButtonSprite.getGlobalBounds();
            }
        }
};



class Clouds
{
    public:
        std::vector<sf::Sprite> clouds;
        sf::Time currTime;
        sf::Texture cloudTexture;
        std::random_device dev;
        std::mt19937 rng{dev()};
        

        Clouds()
        {
            if(cloudTexture.loadFromFile("rsrc/Images/Clouds.png"))
            {
                std::cout << "Loaded CloudTexture" << std::endl;
            }
            clouds.reserve(4);
            clouds.emplace_back(sf::Sprite(cloudTexture));
            clouds.back().setPosition(sf::Vector2f(windowSize_x, windowSize_y/2 - 40.f));
        }

        void updateClouds(sf::Time& deltaTime)
        {
            
            currTime += deltaTime;
            if(currTime.asSeconds() > 8.f)
            {
                clouds.emplace_back(sf::Sprite(cloudTexture));

                std::uniform_int_distribution<std::mt19937::result_type> dist6( windowSize_y/2 - 200, windowSize_y/2 - 50);
                clouds.back().setPosition(sf::Vector2f(windowSize_x, dist6(rng)));
            
                currTime = sf::Time::Zero; 
            }
            

            for(int i = 0; i < clouds.size(); i++)
            {
                if(playerDead == false)
                    clouds[i].move(sf::Vector2f(-1.f, 0.f));
                if(playerDead == true)
                    clouds[i].move(sf::Vector2f(-0.5f, 0.f));

                if(clouds[i].getPosition().x < 0.f - (float)cloudTexture.getSize().x)
                {
                    std::vector<sf::Sprite>::iterator cloudIter = clouds.begin() + i;
                    clouds.erase(cloudIter);
                }
            }
        }
    

        void drawTo(sf::RenderWindow& window)
        {
            for(auto& cloud: clouds)
            {
                window.draw(cloud);
            }
        }
    
};



class GameState
{
    public:
        Fps fps;
        Dino dino;
        Ground ground;
        Obstacles obstacles;
        Scores scores;
        Clouds clouds;
        RestartButton restartButton;
        sf::Font gameOverFont;
        sf::Text gameOverText;
        sf::Vector2f mousePos{0.f, 0.f};
    
        GameState()
        :gameOverText(gameOverFont)
        {
            if(gameOverFont.openFromFile("rsrc/Fonts/Font.ttf"))
            {
                gameOverText.setFont(gameOverFont);
            }
            dino.dino.setPosition(sf::Vector2f(windowSize_x/2 - windowSize_x/4, groundLevel));
            gameOverText.setString("Game Over");
            gameOverText.setPosition(sf::Vector2f(restartButton.restartButtonSprite.getPosition().x - gameOverText.getCharacterSize(),
                                                restartButton.restartButtonSprite.getPosition().y - 50));
            gameOverText.setFillColor(sf::Color(83, 83, 83));
        }
        void setMousePos(sf::Vector2i p_mousePos)
        {
            mousePos.x = p_mousePos.x;
            mousePos.y = p_mousePos.y;
        }

        void update(sf::Time deltaTime)
        {
            restartButton.checkPressed = sf::Mouse::isButtonPressed(sf::Mouse::Button::Left);
            if(playerDead == true && (restartButton.restartButtonSpriteBounds.contains(mousePos) && 
                restartButton.checkPressed == true || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Space)))
            {
                ground.reset();
                obstacles.reset();
                dino.reset();
                scores.reset();
                playerDead = false;
                gameSpeed = 8;
            }
            else
            {
                ground.updateGround();
                obstacles.update(deltaTime, dino);
                dino.update(deltaTime, obstacles.obstacles);
                clouds.updateClouds(deltaTime);
                scores.update();
                scores.updatePowerUpStatus(dino);
            }
            fps.update();
        }   

        void drawTo(sf::RenderWindow& window)
        {
            if(playerDead == true)
            {
                clouds.drawTo(window);
                window.draw(ground.groundSprite);
                obstacles.drawTo(window);
                window.draw(scores.scoresText);
                window.draw(scores.previousScoreText);
                window.draw(scores.HIText);
                window.draw(scores.difficultyText);
                window.draw(scores.speedText);
                window.draw(scores.powerUpStatusText);
                window.draw(scores.sizeText);
                scores.drawPowerUpBars(window, dino);
                window.draw(dino.dino); 
                window.draw(gameOverText);
                window.draw(restartButton.restartButtonSprite);
                fps.drawTo(window); 
            }
            else
            {
                clouds.drawTo(window);
                window.draw(ground.groundSprite);
                obstacles.drawTo(window);
                window.draw(scores.scoresText);
                window.draw(scores.previousScoreText);
                window.draw(scores.HIText);
                window.draw(scores.difficultyText);
                window.draw(scores.speedText);
                window.draw(scores.powerUpStatusText);
                window.draw(scores.sizeText);
                scores.drawPowerUpBars(window, dino);
                window.draw(dino.dino); 
                fps.drawTo(window);    
            }
        }

};


int main()
{
    sf::RenderWindow window(sf::VideoMode({windowSize_x, windowSize_y}), "Google Chrome");
    window.setFramerateLimit(60);

    GameState gameState;

    sf::Clock deltaTimeClock;
    
    while(window.isOpen())
    {
        while(const std::optional<sf::Event> event = window.pollEvent())
        {
            if(event->is<sf::Event::Closed>())
                window.close();
        }
        gameState.setMousePos(sf::Mouse::getPosition(window));
        sf::Time deltaTime = deltaTimeClock.restart();

        gameState.update(deltaTime);

        window.clear(sf::Color::White);
        gameState.drawTo(window);
        window.display();
    }
}