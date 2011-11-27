/* (c) heinrich5991 */

#ifndef GAME_SERVER_GAMEMODES_BOMB_H
#define GAME_SERVER_GAMEMODES_BOMB_H

#include <game/server/gamecontroller.h>

class CGameControllerBOMB : public IGameController
{
public:
	CGameControllerBOMB(class CGameContext *pGameServer);

	virtual bool IsBomb() const { return true; }

	void MakeBomb(int ClientID);
	void MakeRandomBomb();

	virtual void DoWincheck();

	virtual void PostReset();
	virtual void Tick();

	virtual bool OnEntity(int Index, vec2 Pos);
	virtual void OnPlayerInfoChange(class CPlayer *pPlayer);
	virtual void OnCharacterSpawn(class CCharacter *pChr);
	virtual int OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int Weapon);

	struct
	{
		int m_ClientID;
		int m_Tick;
	} m_Bomb;

	bool m_Running;
};

#endif

