// Fill out your copyright notice in the Description page of Project Settings.


#include "Publisher.h"
#include "PubNub.h"

// Sets default values
APublisher::APublisher()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void APublisher::BeginPlay()
{
	Super::BeginPlay();
    enum pubnub_res res;
    pubnub_t* pb = pubnub_alloc();
    if (NULL == pb) {
        return;
    }

    pubnub_init(pb, "demo", "demo");
    pubnub_set_user_id(pb, "my_uuid");

    pubnub_publish(pb, "my_channel", "\"Hello world from UE4!\"");
    res = pubnub_await(pb);

    pubnub_free(pb);
}

// Called every frame
void APublisher::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

