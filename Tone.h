/******************************************************************************
* Tone class
******************************************************************************/
class Tone
{
public:
	Tone(float f1, float f2, char value, char characters[]) :m_f1{ f1 }, m_f2{ f2 }, m_value{ value }, m_characters{ characters } {}

	bool lookup(float * buffer, float numSamples) {

		int indexF1 = m_f1 / (4000.f / numSamples);
		int indexF2 = m_f2 / (4000.f / numSamples);

		if (buffer[indexF1] > m_threshold && buffer[indexF2] > m_threshold)
			return true;
		return false;

	}

	char getValue() const {
		return m_value;
	}

	char getChar(int index) const {
		return m_characters[index-1];
	}

private:
	float m_f1, m_f2;
	char m_value;
	int m_threshold = -30;
	char * m_characters;
};
