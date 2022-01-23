// Fill out your copyright notice in the Description page of Project Settings.


#include "MoveRandomly.h"
#include "MainStructs.h"
#include "DrawDebugHelpers.h" // ue4 code to debug draw stuff
#include "MyMacros.h" // Just a helper macro include to debug print strings easier

// Lets do some simple setup to spawn 10 random subjects
void AMoveRandomly::BeginPlay()
{
	Super::BeginPlay();

	// We need to spawn some subjects initially to perform the logic on
	for (int i = 0; i < NumSubjects; i++)
	{
		// Create a new subject and save a refrence to its handle
		FSubjectHandle NewSubject = SpawnSubject();

		// Create a new position trait for this subject
		FSubjectPosition NewPosition;
		// Set its X/Y randomly
		NewPosition.Value = FVector(FMath::RandRange(-RandomWidth,RandomWidth), FMath::RandRange(-RandomWidth,RandomWidth), 0);

		// In addition, lets give our subject a destination so that it can randomly walk around
		FDestination NewDestination;
		NewDestination.Value = FVector(FMath::RandRange(-RandomWidth,RandomWidth), FMath::RandRange(-RandomWidth,RandomWidth), 0);
		
		// Add the position trait to the subject
		// Calling .SetTrait also adds the trait if it doesn't already exist.
		NewSubject.SetTrait(NewPosition);

		// Add the destination trait to the subject
		NewSubject.SetTrait(NewDestination);

		// Thats it! We created a new subject, and gave it a random position / destination
	}
}

// The following example represents the most core and basic way of using apparatus.
// You require a FFilter, an FChain, and to call Chain->Operate lambda function to apply logic to subjects found with your filter.
void AMoveRandomly::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	
	// Opening brackets like this isnt nessicary but it can be nice for formatting many side by side mechanics.
	// MECHANIC - Find everyone who has finished moving, and update their desired location
	{
		// 1. Declare our filter.
		// All subjects in the game world that currently contain a position and a destination will be caught with this filter
		// Note, this doesnt apply ONLY to position, and destination, an example subject may have 50 traits, but provided...
		// ... that it has specificly these two, then our filter will capture it.
		FFilter Filter = FFilter::Make<FSubjectPosition, FDestination>();

		// It can also be declared like so: 
		// FFilter Filter;
		// Filter.Include<FMyPosition>(); // Traits can also be included like this
		// Filter.Exclude<FMyPosition>(); // Traits can also be excluded like this

		// 2. Declare our Chain
		// This "Enchains" our filter allowing us to uperate
		// The equivilent chain required for multi-threading purposes is FSolidChain. Consult the official doccumentation for more info on Solid Chains.
		FChain* Chain = Enchain(Filter);

		// 3. We "Operate" over our chain
		// The following logic will be ran on every Subject that our Filter caught above.
		// The FSubjectHandle will be the subject that the current iteration of Operate is operating on
		// The FSubjectPosition can be passed in and automatically will be filled with valid data
		// Additional Traits can be passed into the Operation, but be sure they they are valid ahead of time (captured with the filter)
		Chain->Operate([&](FSubjectHandle CurrentSubject, FSubjectPosition CurrentPos, FDestination Destination)
		{
			// For this example, we want all Positions to move toward Destination at a given rate.
			// If they are near the destination, we want to add a trait signalling that their pathing is finished ...
			// So that some other system may pick up / handle this subject
			// NOTE - You dont need to break things up into extra systems, but I encourage you to condier structuring code that way.

			// Get distance from our destination
			float DistFromDestination = FVector::Dist(CurrentPos.Value, Destination.Value);

			// If we're currently far from our destination, then move toward it.
			if (DistFromDestination > MinDistanceFroMDestination) // MinDistanceFroMDestination decalred in .h file.
			{
				// The vector pointing toward destination
				FVector MoveVector = Destination.Value - CurrentPos.Value;

				// Normalize it so that we have a unit vector to work with
				MoveVector.Normalize();

				// Scale the direction by movespeed and delta seconds.
				// In addition, you could consider storing your subjects MoveSpeed in another trait, accessing it via ...
				// ... this filter, and setting your speed that way.
				MoveVector *= DeltaSeconds * MoveSpeed;

				// Lets update the position trait
				FSubjectPosition UpdatedPos;
				UpdatedPos.Value = CurrentPos.Value + MoveVector; // Add the move vector onto our existing position
				CurrentSubject.SetTrait(UpdatedPos);
			}

			// We're close enough to have finished our move.
			else
			{
				// Create our finished moving struct...
				FFinishedMoving FinishedMove;
				
				// Add it to our trait
				// NOTE - Calling "Set Trait" also functions to ADD a given trait OR to override the value of the trait already there
				CurrentSubject.SetTrait(FinishedMove);
			}
		});
	}

	// Lets get a new destination for anyone who has finished their move.
	// MECHANIC - Handles all subjects who have finished their last move.
	{
		// 1. Make a filter of subjects who have finished moving
		FFilter Filter = FFilter::Make<FFinishedMoving>();

		// 2. Make our chain
		FChain* Chain = Enchain(Filter);

		// 3. Operate!
		Chain->Operate([&](FSubjectHandle CurrentSubject)
		{
			// Firstly, lets create a new random destination for the subject
			FDestination NewDestination;
			NewDestination.Value = FVector(FMath::RandRange(-RandomWidth,RandomWidth), FMath::RandRange(-RandomWidth,RandomWidth), 0);

			// Now lets add it to the subject
			CurrentSubject.SetTrait(NewDestination);

			// Lastly, lets remove the finished moving trait, so that this code doesnt call again.
			// Next tick, this subject will continue moving
			CurrentSubject.RemoveTrait<FFinishedMoving>();
		});
	}

	// Lets also create a utility mechanic that draws our destinations
	// MECHANIC - Draws positions of all subjects with positions.
	{
		// 1. Make a filter of only finished moving subjects
		FFilter Filter = FFilter::Make<FDestination>();

		// 2. Make our chain
		FChain* Chain = Enchain(Filter);

		// 3. Operate!
		Chain->Operate([&](FSubjectHandle CurrentSubject, FDestination Destination)
		{
			// Unreal command to draw our positions.
			DrawDebugPoint(GetWorld(), Destination.Value,  20.0f, FColor::Red, false, 0.25f);
		});
	}

	
	// Lastly, lets write a simple system to debug draw our subject positions so we can see what is going on.
	// MECHANIC - Draws positions of all subjects with positions.
	{
		// 1. Make a filter of only finished moving subjects
		FFilter Filter = FFilter::Make<FSubjectPosition>();

		// 2. Make our chain
		FChain* Chain = Enchain(Filter);

		// 3. Operate!
		Chain->Operate([&](FSubjectHandle CurrentSubject, FSubjectPosition CurrentPos)
		{
			// Unreal command to draw our positions.
			DrawDebugPoint(GetWorld(), CurrentPos.Value,  20.0f, FColor::Green, false, 0.01f);
		});
	}
}
