After BruteForceServo.ino, I decided to try and make a servo sketch based on Timer1 in CTC mode. With with some help of Perplexity.AI, ChatGPT, and a lot of polishing, I came up with something that worked ok.
This sketch can drive up to 8 servos, from pins PA0 to PA7.<p>
Timer1 is used to make a ISR run at 50 Hz, and pull the appropriate pins low when needed.
